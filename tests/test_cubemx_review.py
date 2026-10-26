"""Protect the actual source/private files and detect regenerated setting loss."""
import json
from pathlib import Path
import tempfile
import unittest

from firmware.scripts.check_cubemx import check
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


if __name__ == "__main__":
    unittest.main()
