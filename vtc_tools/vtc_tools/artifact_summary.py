import argparse
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Summarize generated artifacts for VTC ROS 2 workflows."
    )
    parser.add_argument(
        "artifact_dir",
        nargs="?",
        default=".",
        help="Directory to inspect for rosbag, map, and report outputs.",
    )
    args = parser.parse_args()

    artifact_dir = Path(args.artifact_dir).resolve()
    print(f"artifact_dir: {artifact_dir}")
    print("status: scaffold only")
    print("next: implement rosbag/map/report discovery rules")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
