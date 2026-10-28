"""Small ordered-window PyTorch classifier used by training and inference."""
from pathlib import Path
import hashlib
import json
import math

import torch
from torch import nn

SENSOR_COLUMNS = (
    "acceleration_x", "acceleration_y", "acceleration_z",
    "gyro_x", "gyro_y", "gyro_z",
)
WINDOW_SIZE = 5
LABELS = {0: "WALKING", 1: "RUNNING"}
FORMAT = "rtspark-pytorch-gru-v1"
HIDDEN_SIZE = 8
ACCEL_LIMIT_G = 2.0
GYRO_LIMIT_RAD_S = math.radians(250.0)


class ActivityGRU(nn.Module):
    """450 trainable parameters, all six axes, and two derived magnitudes."""

    def __init__(self):
        super().__init__()
        self.register_buffer("mean", torch.zeros(8))
        self.register_buffer("scale", torch.ones(8))
        self.gru = nn.GRU(8, HIDDEN_SIZE, batch_first=True)
        self.output = nn.Linear(HIDDEN_SIZE, 2)

    @staticmethod
    def features(raw):
        if raw.ndim != 3 or tuple(raw.shape[1:]) != (WINDOW_SIZE, 6):
            raise ValueError("Expected a batch of five readings with six axes.")
        accel = raw[..., :3].clamp(-ACCEL_LIMIT_G, ACCEL_LIMIT_G)
        gyro = raw[..., 3:].clamp(-GYRO_LIMIT_RAD_S, GYRO_LIMIT_RAD_S)
        return torch.cat((
            accel, gyro,
            torch.linalg.vector_norm(accel, dim=-1, keepdim=True),
            torch.linalg.vector_norm(gyro, dim=-1, keepdim=True),
        ), dim=-1)

    def forward(self, raw):
        values = (self.features(raw) - self.mean) / self.scale
        _, hidden = self.gru(values)
        return self.output(hidden[-1])


def rotate_windows(raw):
    """Apply one uniformly sampled proper 3D rotation per entire window."""
    q = torch.randn(len(raw), 4, dtype=raw.dtype, device=raw.device)
    q = q / torch.linalg.vector_norm(q, dim=1, keepdim=True).clamp_min(1e-12)
    w, x, y, z = q.unbind(1)
    rotations = torch.stack((
        1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w),
        2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w),
        2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y),
    ), dim=1).reshape(-1, 3, 3)
    return torch.cat((
        torch.bmm(raw[..., :3], rotations),
        torch.bmm(raw[..., 3:], rotations),
    ), dim=-1)


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load_model(path=None):
    path = Path(path or Path(__file__).with_name("activity_model.pt"))
    metadata_path = path.with_suffix(".json")
    if not path.is_file() or not metadata_path.is_file():
        raise RuntimeError("PyTorch model missing. Run cloud/classifier/train_model.py first.")
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    if (metadata.get("format") != FORMAT
            or metadata.get("sensor_columns") != list(SENSOR_COLUMNS)
            or metadata.get("window_size") != WINDOW_SIZE
            or metadata.get("hidden_size") != HIDDEN_SIZE
            or metadata.get("labels") != {"0": "WALKING", "1": "RUNNING"}
            or metadata.get("units") != {"acceleration": "g", "gyroscope": "rad/s"}
            or metadata.get("nominal_sample_hz") != 1.0
            or metadata.get("model_sha256") != sha256(path)):
        raise ValueError("Model metadata, units, contract or SHA-256 mismatch.")
    model = ActivityGRU()
    state = torch.load(path, map_location="cpu", weights_only=True)
    if not isinstance(state, dict) or any(
        not isinstance(tensor, torch.Tensor) or not torch.isfinite(tensor).all()
        for tensor in state.values()
    ):
        raise ValueError("Model state contains invalid tensors.")
    model.load_state_dict(state, strict=True)
    if torch.any(model.scale <= 0):
        raise ValueError("Model normalization scale must be positive.")
    model.eval()
    return model, metadata
