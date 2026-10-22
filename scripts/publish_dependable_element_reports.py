import os
import shutil
import sys
from pathlib import Path


def _make_writable(path: Path) -> None:
    for root, dirs, files in os.walk(path):
        for name in dirs + files:
            os.chmod(os.path.join(root, name), 0o755)
    os.chmod(path, 0o755)


def main() -> int:
    workspace_dir = os.environ.get("BUILD_WORKSPACE_DIRECTORY")
    if not workspace_dir:
        print("error: this script must be run via 'bazel run'", file=sys.stderr)
        return 1

    src = (
        Path(__file__).parent.parent
        / "score/mw/log/dependability/dependable_element_mw_log_backend_doc/html"
    )
    dest = (
        Path(workspace_dir)
        / "_build"
        / "dependable_elements"
        / "dependable_element_mw_log_backend_doc"
    )

    if not src.is_dir():
        print(
            f"error: expected dependable_element_mw_log_backend_doc output at {src}",
            file=sys.stderr,
        )
        return 1

    if dest.exists():
        _make_writable(dest)
        shutil.rmtree(dest)
    shutil.copytree(src, dest)
    _make_writable(dest)

    print(f"Published dependable_element_mw_log_backend_doc to {dest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
