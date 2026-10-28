"""Download only the handout's verified Kaggle version 2 CSV (no credentials)."""
import argparse
import hashlib
from io import BytesIO
from pathlib import Path
from urllib.request import Request, urlopen
from zipfile import ZipFile

URL = "https://www.kaggle.com/api/v1/datasets/download/vmalyi/run-or-walk?datasetVersionNumber=2"
CSV_SHA256 = "56ab5e861da22f7b1b4eaefa2d6fdc170f6febc2cf03d5a1883a180dab083271"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("dataset/dataset.csv"))
    args = parser.parse_args()
    if args.output.exists():
        if hashlib.sha256(args.output.read_bytes()).hexdigest() == CSV_SHA256:
            print("Verified dataset version 2 already exists.")
            return
        raise SystemExit("Existing output differs from the verified dataset; choose another --output.")
    request = Request(URL, headers={"User-Agent": "RTSpark-Lab3/1"})
    with urlopen(request, timeout=40) as response:
        archive = response.read(16*1024*1024+1)
    if len(archive) > 16*1024*1024:
        raise SystemExit("Unexpectedly large dataset archive.")
    with ZipFile(BytesIO(archive)) as package:
        info = package.getinfo("dataset.csv")
        if info.file_size > 32*1024*1024:
            raise SystemExit("Unexpectedly large dataset CSV.")
        data = package.read(info)
    if hashlib.sha256(data).hexdigest() != CSV_SHA256:
        raise SystemExit("Dataset SHA-256 mismatch; no output written.")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"Verified Kaggle run-or-walk v2: {args.output} ({len(data)} bytes).")
    print("Creator: Viktor Malyi. Dataset license: CC BY-NC-SA 4.0.")
    print("CSV remains excluded from Git.")


if __name__ == "__main__":
    main()
