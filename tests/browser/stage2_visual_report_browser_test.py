"""Real-browser checks for the generated, self-contained Stage 2 reports.

Run with: PYTHONPATH=/tmp/hrn-playwright python3 tests/browser/stage2_visual_report_browser_test.py
"""

from pathlib import Path
import sys
import unittest

from playwright.sync_api import sync_playwright


REPORTS = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else (
    Path(__file__).resolve().parents[2] / "build-visual-dev" / "reports"
)
if len(sys.argv) > 1:
    del sys.argv[1]


class VisualReportBrowserTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.playwright = sync_playwright().start()
        cls.browser = cls.playwright.chromium.launch(headless=True)

    @classmethod
    def tearDownClass(cls):
        cls.browser.close()
        cls.playwright.stop()

    def setUp(self):
        self.page = self.browser.new_page(viewport={"width": 1440, "height": 900})
        self.errors = []
        self.page.on("pageerror", lambda error: self.errors.append(str(error)))

    def tearDown(self):
        self.assertEqual(self.errors, [])
        self.page.close()

    def open_report(self, name):
        self.page.goto((REPORTS / name).as_uri())

    def active_nodes(self):
        return self.page.locator(".tree-node.is-active").evaluate_all(
            "nodes => nodes.map(node => node.dataset.node)"
        )

    def test_circle_click_has_no_oversized_browser_focus_ring(self):
        self.open_report("stage2-solved.html")
        circle = self.page.locator('.section-circle[data-node="p4"]')
        circle.click()
        self.assertEqual(self.active_nodes(), ["p4"])
        self.assertIn("P5", self.page.locator("#section-detail").inner_text())
        self.assertEqual(circle.evaluate("node => getComputedStyle(node).outlineStyle"), "none")
        self.page.screenshot(path=str(REPORTS / "browser-circle-selected.png"), full_page=True)

    def test_mapping_row_selects_only_its_two_endpoints(self):
        self.open_report("stage2-solved.html")
        rows = self.page.locator(".map-row")
        rows.first.click()
        self.assertEqual(self.active_nodes(), ["p0", "n0"])
        self.assertEqual(self.page.locator(".map-row.is-active").count(), 1)
        self.assertIn("P1", self.page.locator("#section-detail").inner_text())
        self.assertIn("N1", self.page.locator("#section-detail").inner_text())
        self.page.locator('.tree-node[data-node="p0"]').click()
        self.assertEqual(self.active_nodes(), ["p0", "n0", "n1"])
        self.assertEqual(self.page.locator(".map-row.is-active").count(), 2)

    def test_independent_zoom_keyboard_selection_and_failure_page(self):
        self.open_report("stage2-solved.html")
        diagrams = self.page.locator(".diagram")
        left_svg = diagrams.nth(0).locator("svg")
        right_svg = diagrams.nth(1).locator("svg")
        initial_left = left_svg.get_attribute("viewBox")
        initial_right = right_svg.get_attribute("viewBox")
        diagrams.nth(0).locator('[data-action="zoom-in"]').click()
        self.assertNotEqual(left_svg.get_attribute("viewBox"), initial_left)
        self.assertEqual(right_svg.get_attribute("viewBox"), initial_right)
        diagrams.nth(0).locator('[data-action="fit"]').click()
        self.assertEqual(left_svg.get_attribute("viewBox"), initial_left)
        circle = self.page.locator('.section-circle[data-node="n2"]')
        circle.focus()
        circle.press("Enter")
        self.assertEqual(self.active_nodes(), ["p2", "n2"])
        self.open_report("stage1-mapping-conflict.html")
        self.assertEqual(self.page.locator(".section-circle").count(), 0)
        self.assertEqual(self.page.locator(".map-row").count(), 4)
        self.page.locator('.tree-node[data-node="p4"]').click()
        self.assertEqual(self.active_nodes(), ["p4"])
        self.page.locator(".map-row").last.click()
        self.assertEqual(self.active_nodes(), ["p0", "n2"])
        self.assertIn("N3", self.page.locator("#section-detail").inner_text())

    def test_dark_narrow_layout_retains_circle_selection(self):
        self.page.set_viewport_size({"width": 700, "height": 900})
        self.page.emulate_media(color_scheme="dark")
        self.open_report("stage2-solved.html")
        circle = self.page.locator('.section-circle[data-node="n0"]')
        circle.click()
        self.assertEqual(self.active_nodes(), ["p0", "n0"])
        self.assertEqual(circle.evaluate("node => getComputedStyle(node).outlineStyle"), "none")
        self.page.screenshot(path=str(REPORTS / "browser-mobile-dark.png"), full_page=True)

    def test_case_matrix_index_opens_mixed_report(self):
        self.open_report("stage2-case-matrix-index.html")
        self.page.locator('a[href="stage2-roots-n-n-mixed.html"]').click()
        self.assertTrue(self.page.url.endswith("/stage2-roots-n-n-mixed.html"))
        self.page.locator(".map-row").nth(1).click()
        self.assertEqual(self.active_nodes(), ["p3", "n1"])
        detail = self.page.locator("#section-detail").inner_text()
        self.assertIn("mix-p-otm", detail)
        self.assertIn("mix-n-otm-parent", detail)

    def test_mapped_sibling_overlap_reports_keep_both_members(self):
        for name, members in (("stage2-overlap-2-1.html", ("p0", "p1")),
                               ("stage2-overlap-1-2.html", ("n0", "n1"))):
            with self.subTest(case=name):
                self.open_report("stage2-case-matrix-index.html")
                self.page.locator(f'a[href="{name}"]').click()
                self.assertEqual(self.page.locator(".section-circle").count(), 3)
                first, second = [self.page.locator(f'.section-circle[data-node="{m}"]')
                                 for m in members]
                for attribute in ("cx", "cy", "r"):
                    self.assertEqual(first.get_attribute(attribute), second.get_attribute(attribute))
                for member in members:
                    self.page.locator(f'.tree-node[data-node="{member}"]').click()
                    self.assertIn(member, self.active_nodes())


if __name__ == "__main__":
    unittest.main()
