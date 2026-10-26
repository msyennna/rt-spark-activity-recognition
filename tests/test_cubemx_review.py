"""Protect the actual source/private files and detect regenerated setting loss."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from zipfile import ZipFile

from firmware.scripts.check_cubemx import CORE_FILES, FIRMWARE, check, user_blocks
from firmware.scripts.prepare_cubemx_review import prepare


class CubeMXReviewTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.output = Path(self.temp.name)/"activity_recognition"

    def tearDown(self):
        self.temp.cleanup()

    def test_copy_has_only_public_seed_files_and_does_not_claim_generation(self):
        result = prepare(self.output)
        manifest = json.loads((self.output/"rtspark_review_seed.json").read_text())
        self.assertFalse(result["cubemx_generation_performed"])
        expected = {"activity_recognition.ioc", "Core/Src/main.c",
                    "Core/Src/stm32f4xx_hal_msp.c", "Core/Src/stm32f4xx_it.c",
                    "Core/Inc/main.h", "Core/Inc/stm32f4xx_it.h", "rtspark_review_seed.json"}
        self.assertEqual({p.relative_to(self.output).as_posix()
                          for p in self.output.rglob("*") if p.is_file()}, expected)
        self.assertEqual(len(manifest["files"]), 6)
        self.assertEqual(check(self.output), [])

    def test_existing_review_is_never_overwritten(self):
        self.output.mkdir()
        marker = self.output/"keep.txt"
        marker.write_text("original")
        with self.assertRaises(FileExistsError):
            prepare(self.output)
        self.assertEqual(marker.read_text(), "original")

    def test_generated_clock_change_is_rejected_even_when_source_is_valid(self):
        prepare(self.output)
        ioc = self.output/"activity_recognition.ioc"
        ioc.write_text(ioc.read_text().replace("RCC.PLLQ=7", "RCC.PLLQ=5"))
        self.assertEqual(check(), [])
        self.assertTrue(any("generated .ioc: RCC.PLLQ must be 7" in e for e in check(self.output)))

    def test_generated_duplicate_sensor_bus_owner_is_rejected(self):
        prepare(self.output)
        main = self.output/"Core/Src/main.c"
        main.write_text(main.read_text()+"\nI2C_HandleTypeDef hi2c2;\n")
        self.assertTrue(any("generated main.c: remove Cube's duplicate I2C2" in e
                            for e in check(self.output)))

    def test_actual_cubemx_output_preserved_all_application_blocks(self):
        fixture = Path(__file__).parent / "fixtures/cubemx_6_18_1_review.zip"
        self.assertEqual(hashlib.sha256(fixture.read_bytes()).hexdigest(),
                         "fd6e2664658a419215e7fd823d55d2c46adba9280136a8eae8fae3e8cfa75340")
        with ZipFile(fixture) as archive:
            # Compress-Archive uses Windows separators. Extract this immutable,
            # hash-checked fixture consistently on both Windows and Linux.
            for entry in archive.infolist():
                target = self.output / entry.filename.replace("\\", "/")
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(archive.read(entry))
        for active, generated in CORE_FILES.items():
            before = user_blocks((self.output/generated).read_text(), generated)
            after = user_blocks((FIRMWARE/active).read_text(), active)
            for name, body in before.items():
                if body:
                    self.assertEqual(after.get(name), body, (active, name))
        # The missing disabled NVIC key is valid. Both generated I2C owners
        # must be reconciled before accepting this actual CubeMX output.
        self.assertEqual(set(check(self.output)), {
            "generated main.c: remove Cube's duplicate I2C2 handle/init; retain custom_bus.c",
            "generated MSP: remove Cube's I2C2 MSP functions; retain custom_bus.c",
        })

    def test_explicitly_disabled_exti_is_also_accepted(self):
        prepare(self.output)
        ioc = self.output/"activity_recognition.ioc"
        ioc.write_text(ioc.read_text()+"NVIC.EXTI15_10_IRQn=false\\:0\\:0\n")
        self.assertEqual(check(self.output), [])

    def test_enabled_exti_setting_is_rejected(self):
        prepare(self.output)
        ioc = self.output/"activity_recognition.ioc"
        ioc.write_text(ioc.read_text()+"NVIC.EXTI15_10_IRQn=true\\:0\\:0\n")
        self.assertTrue(any("generated .ioc: EXTI15_10 NVIC" in e for e in check(self.output)))

    def test_missing_exti_key_cannot_hide_an_enable_call(self):
        prepare(self.output)
        main = self.output/"Core/Src/main.c"
        main.write_text(main.read_text()+"\nvoid unsafe(void) { HAL_NVIC_EnableIRQ(EXTI15_10_IRQn); }\n")
        self.assertTrue(any("generated C code: EXTI15_10 NVIC" in e for e in check(self.output)))

    def test_commented_exti_enable_does_not_trigger_a_false_positive(self):
        prepare(self.output)
        main = self.output/"Core/Src/main.c"
        main.write_text(main.read_text()+"\n// HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);\n")
        self.assertEqual(check(self.output), [])

    def test_generated_msp_cannot_take_sensor_bus_ownership(self):
        prepare(self.output)
        msp = self.output/"Core/Src/stm32f4xx_hal_msp.c"
        msp.write_text(msp.read_text()+"\nvoid HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {}\n")
        self.assertTrue(any("generated MSP: remove Cube's I2C2 MSP" in e for e in check(self.output)))

    def test_valid_ioc_cannot_hide_a_changed_generated_clock(self):
        prepare(self.output)
        main = self.output/"Core/Src/main.c"
        main.write_text(main.read_text().replace("PLL.PLLQ = 7;", "PLL.PLLQ = 5;"))
        self.assertTrue(any("generated main.c: compiled PLLQ" in e for e in check(self.output)))

    def test_lost_application_block_is_rejected(self):
        prepare(self.output)
        main = self.output/"Core/Src/main.c"
        main.write_text(main.read_text().replace("  rtspark_mqtt_poll();", "  /* accidentally removed */"))
        self.assertTrue(any("active USER CODE 3 was changed/lost" in e for e in check(self.output)))


if __name__ == "__main__":
    unittest.main()
