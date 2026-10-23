import os
import shutil
import stat
import sys
from pathlib import Path

from python.runfiles import Runfiles


def _add_owner_write(path: str) -> None:
    # Skip symlinks: chmod would follow them and modify the target.
    if os.path.islink(path):
        return
    os.chmod(path, stat.S_IMODE(os.stat(path).st_mode) | stat.S_IWUSR)


def _make_writable(path: Path) -> None:
    for root, dirs, files in os.walk(path):
        for name in dirs + files:
            _add_owner_write(os.path.join(root, name))
    _add_owner_write(str(path))


def publish(src: Path, build_dir: Path, name: str) -> int:
    """Copies the pre-built HTML at `src` to `build_dir/dependable_elements/<name>`."""
    # The docs build may wipe _build, so it has to run before this script.
    if not (build_dir / "index.html").is_file():
        print(
            f"error: {build_dir}/index.html not found; run //:docs before publishing",
            file=sys.stderr,
        )
        return 1

    if not src.is_dir():
        print(f"error: expected {name} output at {src}", file=sys.stderr)
        return 1

    # Catches a mismatch between the published folder and the link in docs/dependable_elements.
    landing = build_dir / "dependable_elements" / "index.html"
    link = f'href="{name}/index.html"'
    if not landing.is_file() or link not in landing.read_text(encoding="utf-8"):
        print(f"error: {landing} does not contain {link}", file=sys.stderr)
        return 1

    dest = build_dir / "dependable_elements" / name
    tmp = dest.parent / f".{name}.tmp"
    dest.parent.mkdir(parents=True, exist_ok=True)

    try:
        if tmp.exists():
            _make_writable(tmp)
            shutil.rmtree(tmp)
        # Bazel outputs are read-only; make the copy writable so reruns can replace it.
        shutil.copytree(src, tmp)
        _make_writable(tmp)

        if not (tmp / "index.html").is_file():
            print(f"error: {name}/index.html missing in {src}", file=sys.stderr)
            return 1

        if dest.exists():
            _make_writable(dest)
            shutil.rmtree(dest)
        os.replace(tmp, dest)
    finally:
        if tmp.exists():
            _make_writable(tmp)
            shutil.rmtree(tmp)

    print(f"Published {name} to {dest}")
    return 0


def main() -> int:
    workspace_dir = os.environ.get("BUILD_WORKSPACE_DIRECTORY")
    if not workspace_dir:
        print("error: this script must be run via 'bazel run'", file=sys.stderr)
        return 1

    name = os.environ.get("DEPENDABLE_ELEMENT_NAME")
    html_rlocation = os.environ.get("DEPENDABLE_ELEMENT_HTML")
    if not name or not html_rlocation:
        print(
            "error: DEPENDABLE_ELEMENT_NAME and DEPENDABLE_ELEMENT_HTML must be set",
            file=sys.stderr,
        )
        return 1

    runfiles = Runfiles.Create()
    src = runfiles.Rlocation(html_rlocation) if runfiles else None
    if not src:
        print(f"error: cannot resolve runfile {html_rlocation}", file=sys.stderr)
        return 1

    return publish(Path(src), Path(workspace_dir) / "_build", name)


if __name__ == "__main__":
    sys.exit(main())
