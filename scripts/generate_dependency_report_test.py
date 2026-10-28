# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************
import io
import json
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stderr
from pathlib import Path
from unittest import mock

from scripts import generate_dependency_report as report

_CONFIG = {
    "output": "page.rst",
    "title": "Example Dependencies",
    "document_id": "doc__example_dependencies",
    "safety": "ASIL_B",
    "security": "YES",
    "component": "//comp/...",
    "units": [
        {"name": "Unit A", "scope": "//comp/a/..."},
        {"name": "Unit B", "scope": "//comp/b/..."},
    ],
}


def _xml(*rules):
    """Builds `bazel query --output=xml` output from (label, class, tags, actual)."""
    body = ""
    for label, kind, tags, actual in rules:
        body += f'<rule class="{kind}" name="{label}">'
        body += '<list name="tags">'
        body += "".join(f'<string value="{tag}"/>' for tag in tags)
        body += "</list>"
        if actual:
            body += f'<label name="actual" value="{actual}"/>'
        body += "</rule>"
    return f'<?xml version="1.1" encoding="UTF-8"?><query version="2">{body}</query>'


def _fake_bazel(missing=()):
    """Answers the queries of `unit_section` for the units in `_CONFIG`."""

    def query(expression, output):
        for package in missing:
            report.MISSING_PACKAGES.add(package)
        unit = "a" if "//comp/a/..." in expression else "b"
        if output == "label":
            return {
                "a": "//comp/a:lib\n@@baselibs+//x:y\n@@bazel_tools//tools:z\n",
                "b": "//comp/b:lib\n@@baselibs+//x:y\n@@boost.mp11+//:mp11\n",
            }[unit]
        if expression.startswith("labels(deps"):
            return _xml((f"@baselibs//dep:{unit}", "cc_library", [], None))
        return _xml((f"//comp/{unit}:lib", "cc_library", ["FFI"], None))

    return query


class BazelQueryTest(unittest.TestCase):
    def setUp(self) -> None:
        report.MISSING_PACKAGES.clear()

    def _run(self, returncode, stderr=""):
        result = subprocess.CompletedProcess(
            [], returncode, stdout="out", stderr=stderr
        )
        with mock.patch.object(report.subprocess, "run", return_value=result) as run:
            return report.bazel_query("//x", "label"), run

    def test_returns_stdout_and_keeps_going(self) -> None:
        stdout, run = self._run(0)

        self.assertEqual(stdout, "out")
        self.assertIn("--keep_going", run.call_args.args[0])
        self.assertIn("--output=label", run.call_args.args[0])

    def test_partial_result_collects_missing_packages(self) -> None:
        stderr = (
            "ERROR: no such package 'score/a': BUILD file not found\n"
            "ERROR: no such package '@@repo+//b': BUILD file not found\n"
        )

        stdout, _ = self._run(3, stderr)

        self.assertEqual(stdout, "out")
        self.assertEqual(report.MISSING_PACKAGES, {"score/a", "@@repo+//b"})

    def test_other_failure_exits_with_stderr(self) -> None:
        with self.assertRaises(SystemExit) as context:
            self._run(2, "syntax error")

        self.assertIn("syntax error", str(context.exception.code))


class QueryRulesTest(unittest.TestCase):
    def test_parses_kind_ffi_tag_and_alias(self) -> None:
        xml = _xml(
            ("//a:lib", "cc_library", ["FFI", "other"], None),
            ("//a:plain", "cc_library", ["other"], None),
            ("//a:alias", "alias", [], "//b:lib"),
        )
        with mock.patch.object(report, "bazel_query", return_value=xml):
            rules = report.query_rules("//a/...")

        self.assertEqual(
            rules,
            {
                "//a:lib": ("cc_library", True, None),
                "//a:plain": ("cc_library", False, None),
                "//a:alias": ("alias", False, "//b:lib"),
            },
        )


class ResolveAliasesTest(unittest.TestCase):
    def test_alias_inherits_ffi_of_target(self) -> None:
        rules = {
            "//a:alias": ("alias", False, "//b:lib"),
            "//a:tagged_alias": ("alias", True, "//b:plain"),
        }
        targets = {
            "//b:lib": ("cc_library", True, None),
            "//b:plain": ("cc_library", False, None),
        }
        with mock.patch.object(report, "query_rules", return_value=targets) as query:
            resolved = report.resolve_aliases(rules)

        query.assert_called_once_with("//b:lib + //b:plain")
        self.assertEqual(
            resolved,
            {"//a:alias": ("alias", True), "//a:tagged_alias": ("alias", True)},
        )

    def test_no_query_without_aliases(self) -> None:
        with mock.patch.object(report, "query_rules") as query:
            resolved = report.resolve_aliases({"//a:lib": ("cc_library", False, None)})

        query.assert_not_called()
        self.assertEqual(resolved, {"//a:lib": ("cc_library", False)})


class RepoNameTest(unittest.TestCase):
    def test_normalizes_repository_names(self) -> None:
        cases = {
            "//score/mw/log:lib": "",
            "@@bazel_tools//tools:x": "bazel_tools",
            "@@score_baselibs+//score/os:os": "score_baselibs",
            "@score_baselibs//score/os:os": "score_baselibs",
            "@@score_baselibs++_repo_rules+acl-deb//:acl": "acl-deb",
            "@@boost.mp11+//:mp11": "boost",
        }
        for label, expected in cases.items():
            with self.subTest(label=label):
                self.assertEqual(report.repo_name(label), expected)


class TableTest(unittest.TestCase):
    def test_empty_table(self) -> None:
        self.assertEqual(report.table(["A"], [], "100"), "None.\n")

    def test_list_table(self) -> None:
        self.assertEqual(
            report.table(["A", "B"], [["1", "2"]], "50 50"),
            ".. list-table::\n   :header-rows: 1\n   :widths: 50 50\n\n   * - A\n     - B\n   * - 1\n     - 2\n",
        )


class GenerateTest(unittest.TestCase):
    def _generate(self, config=_CONFIG, missing=()):
        with mock.patch.object(report, "bazel_query", _fake_bazel(missing)):
            return report.generate(config)

    def test_header(self) -> None:
        page = self._generate()

        self.assertIn(".. document:: Example Dependencies\n", page)
        self.assertIn("   :id: doc__example_dependencies\n", page)
        self.assertIn("   :safety: ASIL_B\n", page)
        self.assertIn("   :security: YES\n", page)
        self.assertIn("   :realizes: wp__sw_implementation\n", page)
        self.assertIn("Example Dependencies\n####################\n", page)

    def test_realizes_from_config(self) -> None:
        page = self._generate({**_CONFIG, "realizes": "wp__other"})

        self.assertIn("   :realizes: wp__other\n", page)

    def test_unit_sections(self) -> None:
        page = self._generate()

        self.assertIn("Unit A\n======\n", page)
        self.assertIn("   * - ``//comp/a:lib``\n     - yes\n", page)
        self.assertIn(
            "   * - ``@baselibs//dep:a``\n     - cc_library\n     - no\n", page
        )

    def test_external_repositories(self) -> None:
        page = self._generate()
        repositories = page.split("External Repositories\n", 1)[1]

        self.assertIn("   * - ``baselibs``\n     - Unit A, Unit B\n", repositories)
        self.assertIn("   * - ``boost``\n     - Unit B\n", repositories)
        self.assertNotIn("bazel_tools", repositories)
        self.assertNotIn("````", repositories)

    def test_complete_graph_has_no_warning(self) -> None:
        page = self._generate()

        self.assertNotIn(".. warning::", page)
        self.assertIn("All dependencies are resolved.\n", page)

    def test_incomplete_graph_warns_before_tables(self) -> None:
        page = self._generate(missing=["score/missing"])

        warning = page.index(".. warning::")
        self.assertLess(warning, page.index("Unit A\n======"))
        self.assertIn("``score/missing``", page[warning : page.index("Unit A\n")])
        unresolved = page.split("Unresolved Dependencies\n", 1)[1]
        self.assertIn("   * - ``score/missing``\n", unresolved)
        self.assertNotIn("All dependencies are resolved.", page)

    def test_missing_packages_reset_per_page(self) -> None:
        self._generate(missing=["score/missing"])

        page = self._generate()

        self.assertNotIn(".. warning::", page)


class MainTest(unittest.TestCase):
    def setUp(self) -> None:
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        self.config = self.root / "config.json"
        self.config.write_text(json.dumps(_CONFIG))
        self.page = self.root / _CONFIG["output"]
        for patch in (
            mock.patch.object(report, "ROOT", self.root),
            mock.patch.object(report, "bazel_query", _fake_bazel()),
        ):
            patch.start()
            self.addCleanup(patch.stop)

    def _main(self, *args):
        stderr = io.StringIO()
        with (
            mock.patch.object(sys, "argv", ["prog", str(self.config), *args]),
            redirect_stderr(stderr),
        ):
            report.main()
        return stderr.getvalue()

    def test_writes_page(self) -> None:
        stderr = self._main()

        self.assertIn(".. document:: Example Dependencies", self.page.read_text())
        self.assertEqual(stderr, "")

    def test_check_passes_for_current_page(self) -> None:
        self._main()

        self._main("--check")

    def test_check_fails_for_outdated_page(self) -> None:
        self.page.write_text("old")

        with self.assertRaises(SystemExit) as context:
            self._main("--check")

        self.assertIn("outdated: page.rst", str(context.exception.code))
        self.assertEqual(self.page.read_text(), "old")

    def test_check_fails_for_missing_page(self) -> None:
        with self.assertRaises(SystemExit) as context:
            self._main("--check")

        self.assertIn("outdated: page.rst", str(context.exception.code))
        self.assertFalse(self.page.exists())

    def test_warns_about_incomplete_page(self) -> None:
        with mock.patch.object(report, "bazel_query", _fake_bazel(["score/missing"])):
            stderr = self._main()

        self.assertIn(
            "warning: page.rst is incomplete, missing packages: score/missing", stderr
        )


if __name__ == "__main__":
    unittest.main()
