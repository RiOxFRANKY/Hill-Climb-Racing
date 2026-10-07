"""Export the authoritative NumPy terrain to the JSON resource the game loads.

The game embeds assets/terrain_10km.json through resources.qrc. Re-run this
after replacing data/terrain_10km.npz:

    python tools/export_terrain.py

Coordinates are written unchanged, in metres with height positive up. Each
ground chain stays a separate polyline; chains are never joined across a gap.
"""

import json
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "data" / "terrain_10km.npz"
TARGET = ROOT / "assets" / "terrain_10km.json"


def main() -> int:
    with np.load(SOURCE, allow_pickle=False) as data:
        points = data["points_m"]
        offsets = data["chain_offsets"]
        gaps = data["gap_ranges_m"]
        chains = [points[a:b] for a, b in zip(offsets[:-1], offsets[1:])]

        # Every chain must end exactly at a gap's takeoff and the next must
        # start at its landing; otherwise the gap list and geometry disagree.
        for i, gap in enumerate(gaps):
            if chains[i][-1, 0] != gap[0] or chains[i + 1][0, 0] != gap[1]:
                raise SystemExit(f"gap {i} does not match chain boundaries")

        payload = {
            "units": "metres",
            "height_positive_up": True,
            "pixels_per_metre": float(data["pixels_per_metre"]),
            "horizontal_length_m": float(data["length_m"]),
            "sample_step_m": float(data["sample_step_m"]),
            "spawn_x_m": float(data["spawn_x_m"]),
            "finish_x_m": float(data["finish_x_m"]),
            "gap_ranges_m": gaps.tolist(),
            "section_bounds_m": data["section_bounds_m"].tolist(),
            "section_names": [str(name) for name in data["section_names"]],
            "polylines_m": [chain.tolist() for chain in chains],
        }

    with open(TARGET, "w", encoding="utf-8") as file:
        json.dump(payload, file, separators=(",", ":"), allow_nan=False)

    count = sum(len(chain) for chain in payload["polylines_m"])
    print(f"wrote {TARGET.relative_to(ROOT)}: {len(chains)} chains, {count} points, "
          f"{len(payload['gap_ranges_m'])} gaps")
    return 0


if __name__ == "__main__":
    sys.exit(main())
