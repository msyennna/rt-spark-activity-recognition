"""Prepare a fresh public-file CubeMX review project outside the repository."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from firmware.scripts.check_cubemx import CORE_FILES, FIRMWARE, check


def prepare(output):
    output = Path(output).resolve()
    if output.is_relative_to(ROOT):
        raise ValueError("Choose a new review folder outside the repository, for example in Downloads.")
    errors = check()
    if errors:
        raise ValueError("Source guard failed: " + "; ".join(errors))
    # mkdir must fail if the target exists; never overwrite an earlier review.
    output.mkdir(parents=True, exist_ok=False)
    mapping = {"activity_recognition/activity_recognition.ioc": "activity_recognition.ioc",
               **CORE_FILES}
    files = []
    for source_name, target_name in mapping.items():
        source, target = FIRMWARE/source_name, output/target_name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        digest = hashlib.sha256(target.read_bytes()).hexdigest()
        if digest != hashlib.sha256(source.read_bytes()).hexdigest():
            raise ValueError("Review copy hash mismatch: " + target_name)
        files.append({"source": source_name, "copy": target_name, "sha256": digest})
    errors = check(output)
    if errors:
        raise ValueError("Prepared-copy guard failed: " + "; ".join(errors))
    manifest = {"format": "rtspark-cubemx-review-seed-v1", "stage": "seed_only",
                "generation_performed_by_this_helper": False, "files": files}
    (output/"rtspark_review_seed.json").write_text(json.dumps(manifest, indent=2)+"\n", encoding="utf-8")
    return {"status": "PREPARED", "project_directory": str(output),
            "open_in_cubemx": str(output/"activity_recognition.ioc"),
            "public_files_copied": len(files), "cubemx_generation_performed": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        result = prepare(args.output)
    except (OSError, ValueError) as exc:
        print(f"STOPPED: {exc}", file=sys.stderr)
        return 1
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
