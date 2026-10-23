import os
import tempfile
import unittest
from pathlib import Path

from scripts.publish_dependable_element_reports import publish

_NAME = "element_doc"


class PublishTest(unittest.TestCase):
    def setUp(self) -> None:
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._cleanup, tmp)
        self.root = Path(tmp.name)
        self.build = self.root / "_build"
        self.landing_dir = self.build / "dependable_elements"
        self.dest = self.landing_dir / _NAME

        self.landing_dir.mkdir(parents=True)
        (self.build / "index.html").write_text("main")
        (self.landing_dir / "index.html").write_text(
            f'<a href="{_NAME}/index.html">report</a>'
        )

        self.src = self._make_source(
            "src", {"index.html": "v1", "sub/page.html": "page"}
        )

    def _make_source(self, name: str, files: dict[str, str]) -> Path:
        """Creates a read-only tree, like Bazel outputs."""
        root = self.root / name
        for rel, text in files.items():
            path = root / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
            path.chmod(0o444)
        for dirpath, _, _ in os.walk(root, topdown=False):
            os.chmod(dirpath, 0o555)
        return root

    @staticmethod
    def _cleanup(tmp: tempfile.TemporaryDirectory) -> None:
        for dirpath, dirs, _ in os.walk(tmp.name):
            for d in dirs:
                os.chmod(os.path.join(dirpath, d), 0o755)
        tmp.cleanup()

    def _published_names(self) -> list[str]:
        return sorted(p.name for p in self.landing_dir.iterdir())

    def test_publishes_tree_without_stray_files(self) -> None:
        self.assertEqual(publish(self.src, self.build, _NAME), 0)

        self.assertEqual((self.dest / "index.html").read_text(), "v1")
        self.assertEqual((self.dest / "sub" / "page.html").read_text(), "page")
        self.assertEqual(self._published_names(), sorted(["index.html", _NAME]))

    def test_republish_replaces_previous_output(self) -> None:
        publish(self.src, self.build, _NAME)
        new_src = self._make_source("src_v2", {"index.html": "v2"})

        self.assertEqual(publish(new_src, self.build, _NAME), 0)

        self.assertEqual((self.dest / "index.html").read_text(), "v2")
        self.assertFalse((self.dest / "sub").exists())

    def test_published_files_are_not_executable(self) -> None:
        publish(self.src, self.build, _NAME)

        files = [p for p in self.dest.rglob("*") if p.is_file()]
        self.assertTrue(files)
        for path in files:
            self.assertEqual(path.stat().st_mode & 0o111, 0, path)

    def _assert_rejected(self, src: Path) -> None:
        self.assertEqual(publish(src, self.build, _NAME), 1)
        self.assertFalse(self.dest.exists())

    def test_rejects_when_docs_not_built(self) -> None:
        (self.build / "index.html").unlink()
        self._assert_rejected(self.src)

    def test_rejects_missing_source(self) -> None:
        self._assert_rejected(self.root / "missing")

    def test_rejects_landing_page_without_link_to_report(self) -> None:
        (self.landing_dir / "index.html").write_text('<a href="other/index.html">x</a>')
        self._assert_rejected(self.src)

    def test_rejects_missing_landing_page(self) -> None:
        (self.landing_dir / "index.html").unlink()
        self._assert_rejected(self.src)

    def test_failed_publish_keeps_previous_output(self) -> None:
        publish(self.src, self.build, _NAME)
        without_index = self._make_source("broken", {"page.html": "x"})

        self.assertEqual(publish(without_index, self.build, _NAME), 1)

        self.assertEqual((self.dest / "index.html").read_text(), "v1")
        self.assertEqual(self._published_names(), sorted(["index.html", _NAME]))


if __name__ == "__main__":
    unittest.main()
