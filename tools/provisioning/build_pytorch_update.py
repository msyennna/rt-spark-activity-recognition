"""Package the tested PyTorch cloud update; never include keys or the live DB."""
import argparse
import hashlib
import json
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parents[2]
TARGETS = [
    "cloud/classifier/temporal_model.py", "cloud/classifier/temporal_data.py",
    "cloud/classifier/train_model.py", "cloud/classifier/predictor.py",
    "cloud/classifier/activity_model.pt", "cloud/classifier/activity_model.json",
    "cloud/mqtt/mqtt_classifier.py",
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    paths = sorted(set(TARGETS + [
        "cloud/api/app.py", "cloud/database/database.py",
        "tests/test_temporal_model.py", "tests/test_cloud_temporal_contract.py",
        "tools/provisioning/apply_pytorch_update.py",
    ]))
    records = {name: hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in paths}
    manifest = {"format": "rtspark-pytorch-update-v1", "targets": TARGETS, "sha256": records}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(args.output, "w", ZIP_DEFLATED) as archive:
        for name in paths:
            archive.write(ROOT/name, name)
        archive.writestr("pytorch-update-manifest.json", json.dumps(manifest, indent=2)+"\n")
    print(f"Created {args.output}: {len(paths)} explicit files; no database or private configuration.")


if __name__ == "__main__":
    main()
