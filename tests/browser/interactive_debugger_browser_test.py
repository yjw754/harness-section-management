"""Exercise the local debugger UI in Chromium using the real WASM solver."""

import argparse
import json
import math
from pathlib import Path
import re
import unittest

from playwright.sync_api import expect, sync_playwright

from interactive_debugger_smoke_test import bilateral_case, serve


class DebuggerBrowserCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.origin = cls.enterClassContext(serve(DIRECTORY))
        playwright = cls.enterClassContext(sync_playwright())
        cls.browser = playwright.chromium.launch()
        cls.addClassCleanup(cls.browser.close)

    def setUp(self):
        self.page = self.browser.new_page(viewport={"width": 1440, "height": 900})
        self.page.set_default_timeout(5000)
        self.addCleanup(self.page.close)
        self.page_errors = []
        self.page.on("pageerror", lambda error: self.page_errors.append(str(error)))

    def tearDown(self):
        self.assertEqual(self.page_errors, [])

    def open_page(self):
        response = self.page.goto(self.origin + "/index.html")
        self.assertTrue(response.ok, f"Generated debugger page is missing (HTTP {response.status})")
        self.status_is("solved")

    def status_is(self, state):
        expect(self.page.get_by_test_id("status")).to_have_attribute("data-state", state)

    def select_section(self, identifier):
        self.page.locator(f'li[data-section-id="{identifier}"] > .member-row > .tree-node').click()

    def properties(self, identifier):
        return self.page.locator(f'[data-testid="section-properties"][data-section-id="{identifier}"]')

    def field(self, identifier, name):
        return self.properties(identifier).locator(f'[data-field="{name}"]')

    def relations(self):
        return self.page.get_by_test_id("relation-editor").locator("tbody tr")

    def relation_field(self, index, name):
        return self.relations().nth(index).locator(f'[data-field="{name}"]')

    def add_section(self, side="positive"):
        self.page.get_by_test_id(f"add-section-{side}").click()

    def wrap_solver(self, wrapper):
        pattern = re.compile(re.escape(self.origin) + r"/hrn_interactive_solver\.[0-9a-f]{64}\.js$")

        def wrap(route):
            real_module = json.dumps(route.request.url + "?real=1")
            route.fulfill(content_type="text/javascript",
                          body=f"import createRealSolver from {real_module};\n" + wrapper)

        self.page.route(pattern, wrap)


class EditorTest(DebuggerBrowserCase):
    def test_create_asymmetric_trees_and_edit_selected_member(self):
        self.open_page()
        positive = self.page.get_by_test_id("section-tree-positive")
        negative = self.page.get_by_test_id("section-tree-negative")
        positive.locator('li[data-section-id="p-main"] > div').get_by_role("button", name="+ 子截面").click()
        child = positive.locator('li[data-section-id="p-main"] > ul > li[data-section-id="p-1"]')
        expect(child).to_be_visible()
        child.locator(':scope > div').get_by_role("button", name="+ 子截面").click()
        grandchild = child.locator('li[data-section-id="p-2"]')
        expect(grandchild).to_be_visible()
        self.add_section("negative")
        expect(negative.locator('li[data-section-id]')).to_have_count(2)
        negative.locator('li[data-section-id="n-1"] > div').get_by_role("button", name="+ 子截面").click()
        expect(negative.locator('li[data-section-id="n-1"] > ul > li[data-section-id="n-2"]')).to_be_visible()
        editor = self.page.get_by_test_id("section-properties")
        expect(editor).to_have_attribute("data-section-id", "n-2")
        expect(editor.locator('[data-field="parentId"]')).to_have_value("n-1")
        grandchild.get_by_role("button", name="p-2", exact=True).click()
        expect(editor.locator('[data-field="parentId"]')).to_have_value("p-1")
        editor.locator('[data-field="radius"]').fill("0.5")
        self.status_is("solved")
        expect(self.page.locator('circle[data-section-id="p-2"]')).to_have_attribute("r", "0.5")
        editor.locator('[data-field="parentId"]').select_option("")
        expect(positive.locator(':scope > ul > li[data-section-id="p-2"]')).to_be_visible()

    def test_empty_side_can_be_rebuilt(self):
        self.open_page()
        negative = self.page.get_by_test_id("section-tree-negative")
        negative.get_by_role("button", name="N main (n-main)", exact=True).click()
        editor = self.page.get_by_test_id("section-properties")
        editor.get_by_role("button", name="删除截面").click()
        expect(negative.locator("li")).to_have_count(0)
        expect(negative).to_contain_text("无成员截面")
        expect(editor.locator('[data-field="radius"]')).to_have_count(0)
        self.add_section("negative")
        self.status_is("solved")
        expect(negative.locator(':scope > ul > li[data-section-id="n-1"]')).to_be_visible()
        expect(editor).to_have_attribute("data-section-id", "n-1")

    def test_narrow_viewport_stacks_trees_without_page_overflow(self):
        self.page.set_viewport_size({"width": 390, "height": 900})
        self.open_page()
        self.select_section("p-main")
        positive = self.page.get_by_test_id("section-tree-positive").bounding_box()
        negative = self.page.get_by_test_id("section-tree-negative").bounding_box()
        self.assertLess(positive["y"], negative["y"])
        self.assertEqual(positive["x"], negative["x"])
        self.assertEqual(self.page.evaluate("document.documentElement.scrollWidth"), 390)

    def test_keyboard_add_child_focuses_new_node(self):
        self.open_page()
        tree = self.page.get_by_test_id("section-tree-positive")
        tree.get_by_role("button", name="+ 子截面").press("Enter")
        expect(tree.get_by_role("button", name="p-1", exact=True)).to_be_focused(timeout=1000)

    def count_solver_calls(self):
        # Wrap the module without substituting fake geometry or validity results.
        self.wrap_solver("""
                export default async function () {
                  const real = await createRealSolver();
                  window.debuggerCalls = [];
                  return {solveCase(input) {
                    window.debuggerCalls.push(structuredClone(input));
                    return real.solveCase(input);
                  }};
                }
            """)

    def calls(self):
        return self.page.evaluate("window.debuggerCalls")

    def test_numeric_edits_debounce(self):
        self.count_solver_calls()
        self.page.clock.install(time="2026-11-18T12:00:00")
        self.open_page()
        self.page.clock.pause_at("2026-11-18T12:01:00")
        radius = self.page.get_by_test_id("root-positive-radius")
        self.assertEqual(len(self.calls()), 1)
        radius.fill("21")
        self.page.clock.run_for(150)
        radius.fill("24")
        self.page.clock.run_for(299)
        self.assertEqual(len(self.calls()), 1)
        self.page.clock.run_for(50)
        self.assertEqual(len(self.calls()), 2)
        self.assertEqual(self.calls()[-1]["roots"]["positive"]["shape"]["radius"], 24)

        self.select_section("p-main")
        label = self.field("p-main", "label")
        label.fill("正侧主截面")
        expect(label).to_be_focused()
        self.page.get_by_test_id("calculate").click()
        self.page.clock.run_for(350)
        self.assertEqual(len(self.calls()), 3)
        self.assertEqual(self.calls()[-1]["sections"][0]["label"], "正侧主截面")

        self.page.get_by_test_id("auto-calculate").uncheck()
        radius.fill("25")
        self.page.clock.run_for(400)
        self.assertEqual(len(self.calls()), 3)
        expect(radius).to_have_value("25")
        self.page.get_by_test_id("auto-calculate").check()
        radius.fill("")
        self.page.clock.run_for(400)
        self.status_is("invalid")
        expect(self.page.get_by_test_id("diagnostics")).to_contain_text("roots.positive.shape.radius")
        self.assertEqual(len(self.calls()), 3)

    def test_rename_updates_references_atomically(self):
        self.open_page()
        self.add_section()
        self.field("p-1", "parentId").select_option("p-main")
        self.select_section("p-main")
        identifier = self.field("p-main", "id")
        identifier.fill("正-主")
        identifier.press("Enter")
        self.select_section("p-1")
        expect(self.field("p-1", "parentId")).to_have_value("正-主")
        expect(self.relation_field(0, "positiveId")).to_have_value("正-主")
        self.select_section("正-主")
        identifier = self.field("正-主", "id")
        identifier.fill("p-1")
        identifier.press("Enter")
        expect(self.field("正-主", "id")).to_have_value("正-主")
        expect(self.page.get_by_test_id("diagnostics")).to_contain_text("重复")
        expect(self.relation_field(0, "positiveId")).to_have_value("正-主")

    def test_rename_keeps_keyboard_and_pointer_navigation(self):
        self.open_page()
        self.select_section("p-main")
        identifier = self.field("p-main", "id")
        identifier.fill("renamed")
        identifier.press("Tab")
        label = self.field("renamed", "label")
        expect(label).to_be_focused(timeout=1000)
        label.fill("主截面")
        expect(self.relation_field(0, "positiveId").locator("option:checked")).to_have_text("主截面 (renamed)")

        identifier = self.field("renamed", "id")
        identifier.fill("renamed-again")
        self.field("renamed", "radius").click()
        expect(self.field("renamed-again", "radius")).to_be_focused()

        self.field("renamed-again", "id").fill("third-name")
        self.page.get_by_test_id("tree-negative").get_by_role("button", name="N main (n-main)").click()
        expect(self.field("n-main", "id")).to_have_value("n-main", timeout=1000)
        expect(self.page.get_by_test_id("tree-positive").get_by_role("button", name="主截面 (third-name)")).to_be_visible()

    def test_referenced_side_move_and_delete(self):
        self.open_page()
        self.select_section("n-main")
        self.field("n-main", "side").select_option("positive")
        expect(self.field("n-main", "side")).to_have_value("negative")
        self.add_section()
        self.field("p-1", "parentId").select_option("p-main")
        self.field("p-1", "side").select_option("negative")
        expect(self.field("p-1", "side")).to_have_value("positive")
        self.add_section()
        self.field("p-2", "parentId").select_option("p-1")
        self.relations().first.get_by_role("button", name="删除").click()
        self.select_section("p-main")
        self.field("p-main", "side").select_option("negative")
        expect(self.field("p-main", "side")).to_have_value("positive")
        expect(self.page.get_by_test_id("diagnostics")).to_contain_text("引用")
        self.page.get_by_test_id("add-relation").click()
        expect(self.relation_field(0, "positiveId")).to_have_value("p-main")
        self.properties("p-main").get_by_role("button", name="删除截面").click()
        self.select_section("p-1")
        expect(self.field("p-1", "parentId")).to_have_value("")
        self.select_section("p-2")
        expect(self.field("p-2", "parentId")).to_have_value("p-1")
        expect(self.relations()).to_have_count(0)
        expect(self.page.get_by_test_id("diagnostics")).to_contain_text("已删除")

    def test_circular_fields_and_mapping_controls(self):
        self.count_solver_calls()
        self.open_page()
        expect(self.page.get_by_test_id("root-positive-radius")).to_have_value("20")
        self.select_section("p-main")
        expect(self.field("p-main", "radius")).to_have_value("8")
        expect(self.page.locator('[data-field="kind"]')).to_have_count(0)
        self.add_section()
        self.field("p-1", "parentId").select_option("p-main")
        expect(self.field("p-1", "parentId").locator('option[value="n-main"]')).to_have_count(0)
        self.page.get_by_test_id("add-relation").click()
        expect(self.relation_field(1, "positiveId").locator('option[value="n-main"]')).to_have_count(0)
        expect(self.relation_field(1, "negativeId").locator('option[value="p-main"]')).to_have_count(0)
        expect(self.relation_field(1, "samePosition")).to_have_value("true")
        self.relation_field(1, "samePosition").select_option("false")
        self.status_is("solved")
        self.assertEqual(self.calls()[-1]["relations"][1],
                         {"positiveId": "p-1", "negativeId": "n-main", "samePosition": False})


class IntegrationTest(DebuggerBrowserCase):
    def circle(self, identifier):
        return self.page.locator(f'circle[data-section-id="{identifier}"]')

    def tree_node(self, identifier):
        return self.page.locator(f'#results .tree-node[data-section-id="{identifier}"]')

    def import_json(self, content):
        self.page.get_by_test_id("case-import").set_input_files({
            "name": "case.json", "mimeType": "application/json", "buffer": content.encode(),
        })

    def add_nested_child(self):
        self.add_section()
        self.field("p-1", "parentId").select_option("p-main")
        self.field("p-1", "radius").fill("3")

    def center(self, identifier):
        circle = self.circle(identifier)
        return tuple(float(circle.get_attribute(axis)) for axis in ("cx", "cy"))

    def same_side_case(self):
        case = Path(__file__).resolve().parents[2] / "docs/verification/same_side.json"
        return json.loads(case.read_text(encoding="utf-8"))

    def test_linked_children_follow_rotation_deltas_on_either_side(self):
        self.open_page()
        self.import_json(json.dumps(self.same_side_case()))
        self.status_is("solved")

        for root, new_angle, delta, peer, peer_angle in (
                ("n-main", "135", 45, "p-main", "0"),
                ("p-main", "-30", -30, "n-main", "135")):
            with self.subTest(root=root):
                previous = [self.center(f"p-{index}") for index in range(1, 4)]
                self.select_section(root)
                self.field(root, "rotation").fill(new_angle)
                self.status_is("solved")
                angle = math.radians(-delta)
                for index, (x, y) in enumerate(previous, start=1):
                    expected = (x * math.cos(angle) - y * math.sin(angle),
                                x * math.sin(angle) + y * math.cos(angle))
                    for side in ("p", "n"):
                        for actual, target in zip(self.center(f"{side}-{index}"), expected):
                            self.assertAlmostEqual(actual, target, delta=1e-9)
                self.select_section(peer)
                expect(self.field(peer, "rotation")).to_have_value(peer_angle)

        previous = self.center("n-1")
        self.page.get_by_test_id("calculate").click()
        self.status_is("solved")
        self.assertEqual(self.center("n-1"), previous)

    def test_active_negative_side_root_moves_linked_top_level_members(self):
        self.open_page()
        case = self.same_side_case()
        case["sections"] = [section for section in case["sections"] if section["parentId"]]
        for section in case["sections"]:
            section["parentId"] = None
        case["relations"] = case["relations"][1:]
        self.import_json(json.dumps(case))
        self.status_is("solved")
        x, y = self.center("n-1")

        self.page.get_by_test_id("root-negative-rotation").fill("90")
        self.status_is("solved")
        for identifier in ("p-1", "n-1"):
            actual_x, actual_y = self.center(identifier)
            self.assertAlmostEqual(actual_x, y, delta=1e-9)
            self.assertAlmostEqual(actual_y, -x, delta=1e-9)
        expect(self.page.get_by_test_id("root-positive-rotation")).to_have_value("0")

    def test_ordinary_correspondence_does_not_follow_rotation(self):
        self.open_page()
        case = self.same_side_case()
        for relation in case["relations"][1:]:
            relation["samePosition"] = False
        self.import_json(json.dumps(case))
        self.status_is("solved")
        positive = self.center("p-1")
        x, y = self.center("n-1")

        self.select_section("n-main")
        self.field("n-main", "rotation").fill("180")
        self.status_is("solved")
        self.assertEqual(self.center("p-1"), positive)
        actual_x, actual_y = self.center("n-1")
        self.assertAlmostEqual(actual_x, y, delta=1e-9)
        self.assertAlmostEqual(actual_y, -x, delta=1e-9)

    def test_import_rejects_a_modeling_tolerance_the_solver_does_not_use(self):
        self.open_page()
        draft = bilateral_case()
        draft["modelingTolerance"] = 1e-12
        self.import_json(json.dumps(draft))
        expect(self.page.get_by_test_id("diagnostics")).to_contain_text("建模公差")
        self.status_is("solved")

    def test_side_root_rotation_reflows_blocking_unmapped_sibling(self):
        self.open_page()
        fixture = Path(__file__).resolve().parents[2] / "docs/verification/cases/same_side_rot.json"
        self.import_json(fixture.read_text(encoding="utf-8"))
        expect(self.relations()).to_have_count(1)
        self.status_is("solved")

        for side, peer, driver, angle, delta, peer_angle in (
                ("positive", "negative", "p", 90, 45, 45),
                ("negative", "positive", "n", 225, 180, 90)):
            with self.subTest(side=side):
                main_before = self.center("p-main")
                free_before = self.center(f"{driver}-1")
                radians = math.radians(-delta)

                def rotated(center):
                    x, y = center
                    return (x * math.cos(radians) - y * math.sin(radians),
                            x * math.sin(radians) + y * math.cos(radians))

                self.page.get_by_test_id(f"root-{side}-rotation").fill(str(angle))
                self.status_is("solved")
                expect(self.page.get_by_test_id(f"root-{peer}-rotation")).to_have_value(str(peer_angle))
                for prefix in ("p", "n"):
                    main = self.center(f"{prefix}-main")
                    free = self.center(f"{prefix}-1")
                    self.assertLessEqual(math.dist(main, rotated(main_before)), 1e-9)
                    self.assertGreaterEqual(math.dist(main, free), 8 - 1e-9)
                    self.assertLessEqual(max(math.hypot(*main), math.hypot(*free)) + 4, 20 + 1e-9)
                self.assertLessEqual(math.dist(self.center(f"{driver}-1"), rotated(free_before)), 1e-9)

    def test_linked_rotation_collision_rotates_both_sides_and_keeps_the_new_baseline(self):
        self.open_page()
        case = self.same_side_case()
        case["relations"] = case["relations"][:2]
        self.import_json(json.dumps(case))
        self.status_is("solved")
        before = {section["id"]: self.center(section["id"]) for section in case["sections"]}

        self.select_section("n-main")
        self.field("n-main", "rotation").fill("270")
        self.status_is("solved")
        expect(self.page.get_by_test_id("diagnostics")).to_contain_text("已整体旋转")
        expect(self.page.get_by_test_id("layout-positive")).to_have_attribute("data-stale", "false")
        for node, (x, y) in before.items():
            self.assertLessEqual(math.dist(self.center(node), (-x, -y)), 1e-9)
        rotated = {node: self.center(node) for node in before}

        self.page.get_by_test_id("calculate").click()
        self.status_is("solved")
        self.assertEqual({node: self.center(node) for node in before}, rotated)

        self.select_section("p-1")
        self.field("p-1", "rotation").fill("25")
        self.status_is("solved")
        expect(self.page.get_by_test_id("selection-details")).to_contain_text("绝对旋转 205°")
        self.assertEqual({node: self.center(node) for node in before}, rotated)
        self.select_section("p-main")
        expect(self.field("p-main", "rotation")).to_have_value("0")
        self.select_section("n-main")
        expect(self.field("n-main", "rotation")).to_have_value("270")

    def test_large_tangent_layout_can_use_rigid_fallback_repeatedly(self):
        self.open_page()
        case = self.same_side_case()
        case["relations"] = case["relations"][:2]
        for section in [*case["roots"].values(), *case["sections"]]:
            section["shape"]["radius"] *= 10_000_000
        self.import_json(json.dumps(case))
        self.status_is("solved")
        first = self.center("p-1")
        self.select_section("n-main")
        for delta in (20, 40, 60, 80):
            self.field("n-main", "rotation").fill(str(90 + delta))
            self.status_is("solved")
            expect(self.page.get_by_test_id("diagnostics")).to_contain_text("已整体旋转")
            radians = math.radians(-delta)
            x, y = first
            expected = (x * math.cos(radians) - y * math.sin(radians),
                        x * math.sin(radians) + y * math.cos(radians))
            for identifier in ("p-1", "n-1"):
                self.assertLessEqual(math.dist(self.center(identifier), expected), 1e-7)

    def test_manual_rotation_ignores_inactive_root_and_full_turn_as_fallback_drivers(self):
        self.open_page()
        case = self.same_side_case()
        case["relations"] = case["relations"][:2]
        self.import_json(json.dumps(case))
        self.status_is("solved")
        x, y = self.center("p-1")

        self.page.get_by_test_id("auto-calculate").uncheck()
        radius = self.page.get_by_test_id("root-positive-radius")
        radius.fill("21")
        radius.fill("20")
        self.select_section("n-main")
        self.field("n-main", "rotation").fill("270")
        self.select_section("p-1")
        self.field("p-1", "rotation").fill("360")
        self.page.get_by_test_id("root-positive-rotation").fill("30")
        self.status_is("manual")
        self.page.get_by_test_id("calculate").click()
        self.status_is("solved")
        expect(self.page.get_by_test_id("diagnostics")).to_contain_text("已整体旋转")
        for identifier in ("p-1", "n-1"):
            self.assertLessEqual(math.dist(self.center(identifier), (-x, -y)), 1e-9)

    def test_rotation_of_previously_solved_large_unlinked_layout(self):
        self.open_page()
        draft = bilateral_case()
        draft["relations"] = []
        draft["roots"]["positive"]["shape"]["radius"] = 1_000_000_000
        draft["sections"][0]["shape"]["radius"] = 268_435_456
        draft["sections"][1] = {**draft["sections"][0], "id": "p-shell",
                                "shape": {"kind": "circular", "radius": 3}}
        for identifier in ("p-leaf-a", "p-leaf-b"):
            draft["sections"].append({**draft["sections"][1], "id": identifier, "parentId": "p-shell",
                                      "shape": {"kind": "circular", "radius": 1.2}})
        self.import_json(json.dumps(draft))
        expect(self.circle("p-leaf-a")).to_have_count(1)
        self.status_is("solved")

        self.select_section("p-leaf-a")
        self.field("p-leaf-a", "rotation").fill("10")
        self.status_is("solved")
        expect(self.page.get_by_test_id("layout-positive")).to_have_attribute("data-stale", "false")
        self.page.get_by_test_id("calculate").click()
        self.status_is("solved")

    def test_equivalent_angle_reentry_does_not_change_the_fallback_driver(self):
        self.open_page()
        case = self.same_side_case()
        case["relations"] = case["relations"][:2]
        self.import_json(json.dumps(case))
        self.status_is("solved")
        x, y = self.center("p-1")

        self.page.get_by_test_id("auto-calculate").uncheck()
        for identifier, angle in (("n-main", "270"), ("p-1", "45"), ("n-main", "630")):
            self.select_section(identifier)
            self.field(identifier, "rotation").fill(angle)
        self.status_is("manual")
        self.page.get_by_test_id("calculate").click()
        self.status_is("solved")
        expect(self.page.get_by_test_id("diagnostics")).to_contain_text("已整体旋转")
        component = math.sqrt(0.5)
        expected = ((x + y) * component, (y - x) * component)
        self.assertLessEqual(math.dist(self.center("p-1"), expected), 1e-9)

    def test_imported_repeated_endpoint_overlap_keeps_members_selectable(self):
        self.open_page()
        cases = Path(__file__).resolve().parents[2] / "docs/verification/cases"
        for name, identifiers in (("N_1.json", ("p-main", "p-1", "n-main")),
                                   ("1_N.json", ("n-main", "n-1", "p-main"))):
            with self.subTest(case=name):
                self.import_json((cases / name).read_text(encoding="utf-8"))
                self.status_is("solved")
                expect(self.relations()).to_have_count(2)
                for identifier in identifiers:
                    expect(self.circle(identifier)).to_have_attribute("r", "8")
                    self.assertLessEqual(math.dist(self.center(identifier), (0, 0)), 1e-9)
                    self.tree_node(identifier).click()
                    expect(self.tree_node(identifier)).to_have_attribute("aria-pressed", "true")

    def test_asymmetric_parent_repair_from_original_case(self):
        self.open_page()
        fixture = (Path(__file__).resolve().parents[2] / "docs/verification/cases"
                   / "partial_nested.json")
        self.import_json(fixture.read_text(encoding="utf-8"))
        expect(self.relations()).to_have_count(3)
        self.status_is("solved")
        for side in ("positive", "negative"):
            expect(self.page.get_by_test_id(f"root-{side}-rotation")).to_have_value("0")
            expect(self.page.get_by_test_id(f"root-{side}-radius")).to_have_value("20")
        for index in range(3):
            self.assertLessEqual(math.dist(self.center(f"p-{index}"), self.center(f"n-{index}")), 1e-9)
        for parent, children, separate in (("p-3", ("p-0", "p-1"), "p-2"),
                                           ("n-3", ("n-1", "n-2"), "n-0")):
            radius = float(self.circle(parent).get_attribute("r"))
            self.assertAlmostEqual(radius, max(math.dist(self.center(parent), self.center(child))
                                                for child in children) + 3, delta=1e-9)
            self.assertGreaterEqual(math.dist(self.center(parent), self.center(separate)), radius + 3 - 1e-9)
            self.assertLessEqual(math.dist((0, 0), self.center(parent)) + radius, 20 + 1e-9)

    def test_rotating_side_roots_repairs_asymmetric_linked_parents(self):
        self.open_page()
        fixture = (Path(__file__).resolve().parents[2] / "docs/verification/cases"
                   / "partial_nested.json")
        self.import_json(fixture.read_text(encoding="utf-8"))
        expect(self.relations()).to_have_count(3)
        self.status_is("solved")

        for side, peer, degrees in (("positive", "negative", 45), ("negative", "positive", 90)):
            with self.subTest(side=side):
                previous = [self.center(f"p-{index}") for index in range(3)]
                angle = math.radians(-degrees)
                self.page.get_by_test_id(f"root-{side}-rotation").fill(str(degrees))
                self.status_is("solved")
                expect(self.page.get_by_test_id(f"root-{peer}-rotation")).to_have_value(
                    "0" if side == "positive" else "45")

                for index, (x, y) in enumerate(previous):
                    expected = (x * math.cos(angle) - y * math.sin(angle),
                                x * math.sin(angle) + y * math.cos(angle))
                    for prefix in ("p", "n"):
                        self.assertLessEqual(math.dist(self.center(f"{prefix}-{index}"), expected), 1e-9)
                for parent, children, separate in (("p-3", ("p-0", "p-1"), "p-2"),
                                                   ("n-3", ("n-1", "n-2"), "n-0")):
                    center = self.center(parent)
                    radius = float(self.circle(parent).get_attribute("r"))
                    self.assertAlmostEqual(radius, 6, delta=1e-9)
                    for child in children:
                        self.assertLessEqual(math.dist(center, self.center(child)) + 3, radius + 1e-9)
                    self.assertGreaterEqual(math.dist(center, self.center(separate)), radius + 3 - 1e-9)
                    self.assertLessEqual(math.hypot(*center) + radius, 20 + 1e-9)

    def test_singleton_follows_mapping_without_repacking(self):
        self.open_page()
        fixture = Path(__file__).resolve().parents[2] / "docs/verification/cases/stage2-singleton-follows.json"
        self.import_json(fixture.read_text(encoding="utf-8"))
        self.status_is("solved")
        root = self.page.get_by_test_id("layout-positive").locator(".side-root:not(.is-reference)")

        def check_fixed_root():
            self.assertEqual(tuple(float(root.get_attribute(axis)) for axis in ("cx", "cy", "r")),
                             (0, 0, 2.16))

        check_fixed_root()
        original = {identifier: self.center(identifier) for identifier in ("p0", "p1", "p2")}
        self.assertAlmostEqual(math.dist(original["p0"], original["p1"]), 2, delta=1e-9)
        self.assertAlmostEqual(math.dist(original["p1"], original["p2"]), 2, delta=1e-9)
        self.assertAlmostEqual(math.dist(original["p0"], original["p2"]), 2, delta=1e-9)
        self.assertEqual(self.center("n0"), original["p2"])
        for endpoint in ("p0", "p1"):
            self.relation_field(0, "positiveId").select_option(endpoint)
            self.status_is("solved")
            check_fixed_root()
            self.assertEqual(self.center("n0"), original[endpoint])
            for identifier, center in original.items():
                self.assertEqual(self.center(identifier), center)
        self.relations().first.get_by_role("button", name="删除").click()
        self.status_is("solved")
        check_fixed_root()
        self.assertEqual(self.center("n0"), (0, 0))
        for identifier, center in original.items():
            self.assertEqual(self.center(identifier), center)

    def test_active_root_parameters_change_solved_layout(self):
        self.open_page()
        self.relations().first.get_by_role("button", name="删除").click()
        self.add_section("positive")
        self.status_is("solved")
        self.page.get_by_test_id("root-positive-radius").fill("2")
        self.status_is("unsolved")
        expect(self.page.get_by_test_id("diagnostics")).to_contain_text("FixedBoundaryInsufficient")

        auto_resize = self.page.get_by_test_id("root-positive-auto-resize")
        expect(auto_resize).to_be_visible(timeout=1000)
        auto_resize.check()
        self.status_is("solved")
        root = self.page.get_by_test_id("layout-positive").locator(".side-root")
        self.assertAlmostEqual(float(root.get_attribute("r")), 9, delta=1e-9)
        expect(self.page.get_by_test_id("root-positive-radius")).to_have_value("2")

        self.page.get_by_test_id("root-positive-rotation").fill("90")
        self.status_is("solved")
        start, end = self.center("p-main"), self.center("p-1")
        self.assertAlmostEqual(end[0] - start[0], 0, delta=1e-9)
        self.assertAlmostEqual(end[1] - start[1], -9, delta=1e-9)  # SVG Y is inverted.

        self.page.get_by_test_id("root-positive-spacing").fill("2")
        self.status_is("solved")
        self.assertAlmostEqual(math.dist(self.center("p-main"), self.center("p-1")), 11, delta=1e-9)
        expect(self.page.get_by_test_id("layout-negative").locator(".side-root")).to_have_attribute("r", "20")

    def test_root_parameters_round_trip_and_validate_per_side(self):
        self.open_page()
        edits = {"positive": ("23", "25", "0.5", False), "negative": ("30", "-35", "1.25", True)}
        fields = ("radius", "rotation", "spacing", "auto-resize")
        for side, values in edits.items():
            for field, value in zip(fields, values):
                control = self.page.get_by_test_id(f"root-{side}-{field}")
                expect(control).to_be_visible(timeout=1000)
                if isinstance(value, bool):
                    control.set_checked(value)
                else:
                    control.fill(value)
        self.status_is("solved")
        expect(self.page.get_by_test_id("layout-negative").locator(".side-root")).to_have_attribute("r", "30")
        self.assertEqual(self.center("p-main"), (0, 0))  # Only one top-level member: Root is inactive.

        spacing = self.page.get_by_test_id("root-negative-spacing")
        spacing.fill("-1")
        self.status_is("invalid")
        expect(spacing).to_have_attribute("aria-invalid", "true")
        expect(self.page.get_by_test_id("diagnostics")).to_contain_text("roots.negative.spacing")
        spacing.fill("1.25")
        self.status_is("solved")

        with self.page.expect_download() as download:
            self.page.get_by_test_id("case-export").click()
        saved = download.value.path().read_text(encoding="utf-8")
        draft = json.loads(saved)
        self.assertEqual(draft["roots"], {
            "positive": {"shape": {"kind": "circular", "radius": 23},
                         "rotation": 25, "spacing": 0.5, "autoResize": False},
            "negative": {"shape": {"kind": "circular", "radius": 30},
                         "rotation": -35, "spacing": 1.25, "autoResize": True},
        })
        self.open_page()
        self.import_json(saved)
        for side, values in edits.items():
            for field, value in zip(fields, values):
                control = self.page.get_by_test_id(f"root-{side}-{field}")
                if value is True:
                    expect(control).to_be_checked()
                elif value is False:
                    expect(control).not_to_be_checked()
                else:
                    expect(control).to_have_value(value)
        self.status_is("solved")

    def test_edit_and_round_trip(self):
        self.open_page()
        self.page.get_by_test_id("root-positive-radius").fill("24")
        self.add_nested_child()
        self.page.get_by_test_id("add-relation").click()
        self.status_is("solved")
        child_center = self.center("p-1")
        for a, b in zip(child_center, self.center("n-main")):
            self.assertAlmostEqual(a, b, delta=1e-9)
        expect(self.tree_node("p-1")).to_have_attribute("data-depth", "1")
        expect(self.tree_node("n-main")).to_have_attribute("data-depth", "0")
        self.tree_node("p-1").click()
        expect(self.circle("p-1")).to_have_class("section-circle is-selected")
        expect(self.page.get_by_test_id("selection-details")).to_contain_text("p-1")
        expect(self.field("p-1", "radius")).to_have_value("3")

        layout = self.page.get_by_test_id("layout-positive")
        svg = layout.locator("svg")
        fit_viewbox = svg.get_attribute("viewBox")
        layout.get_by_role("button", name="放大").click()
        self.assertNotEqual(svg.get_attribute("viewBox"), fit_viewbox)
        layout.get_by_role("button", name="适应").click()
        self.assertEqual(svg.get_attribute("viewBox"), fit_viewbox)

        with self.page.expect_download() as download_info:
            self.page.get_by_test_id("case-export").click()
        saved = download_info.value.path().read_text(encoding="utf-8")
        draft = json.loads(saved)
        self.assertEqual(draft["roots"]["positive"]["shape"], {"kind": "circular", "radius": 24})
        child = next(entry for entry in draft["sections"] if entry["id"] == "p-1")
        self.assertEqual(child["shape"], {"kind": "circular", "radius": 3})
        self.assertEqual(child["parentId"], "p-main")
        self.assertIn({"positiveId": "p-1", "negativeId": "n-main", "samePosition": True},
                      draft["relations"])

        self.open_page()
        self.import_json(saved)
        expect(self.page.get_by_test_id("root-positive-radius")).to_have_value("24")
        self.status_is("solved")
        self.select_section("p-1")
        expect(self.field("p-1", "parentId")).to_have_value("p-main")
        expect(self.relation_field(1, "positiveId")).to_have_value("p-1")
        for a, b in zip(child_center, self.center("p-1")):
            self.assertAlmostEqual(a, b, delta=1e-9)
        self.assertEqual(self.center("p-1"), self.center("n-main"))

    def test_fit_uses_solved_bounds_below_one_unit(self):
        self.open_page()
        draft = bilateral_case()
        for root in draft["roots"].values():
            root["shape"]["radius"] = 20e-6
        for member in draft["sections"]:
            member["shape"]["radius"] = 8e-6
        self.import_json(json.dumps(draft))
        for side in ("positive", "negative"):
            layout = self.page.get_by_test_id(f"layout-{side}")
            expect(layout.locator(".side-root")).to_have_attribute("r", "0.00002")
            layout.get_by_role("button", name="适应").click()
            width = float(layout.locator("svg").get_attribute("viewBox").split()[2])
            self.assertGreaterEqual(width, 40e-6)
            self.assertLess(width, 60e-6)

    def test_rejected_import_does_not_change_the_case(self):
        self.open_page()
        self.page.get_by_test_id("root-positive-radius").fill("24")
        self.status_is("solved")
        self.tree_node("p-main").click()
        svg = self.page.get_by_test_id("layout-positive").locator("svg")
        solved_before = svg.inner_html()
        unknown, ellipse, rectangle, numeric = (bilateral_case() for _ in range(4))
        unknown["schemaVersion"] = 2
        ellipse["roots"]["negative"]["shape"] = {"kind": "elliptical", "semiMajorAxis": 2,
                                                 "semiMinorAxis": 1}
        rectangle["sections"][0]["shape"] = {"kind": "rectangular", "longSide": 2, "shortSide": 1}
        numeric["sections"][0]["shape"]["radius"] = "8"
        overflow = json.dumps(bilateral_case()).replace('"radius": 20', '"radius": 1e400', 1)
        rejected = [("{", "$"), (json.dumps(unknown), "schemaVersion"),
                    (json.dumps(ellipse), "roots.negative.shape.kind"),
                    (json.dumps(rectangle), "sections[0].shape.kind"),
                    (json.dumps(numeric), "sections[0].shape.radius"),
                    (overflow, "roots.positive.shape.radius")]
        for contents, path in rejected:
            with self.subTest(path=path):
                self.import_json(contents)
                expect(self.page.get_by_test_id("diagnostics")).to_contain_text(path)
                self.status_is("solved")
                expect(self.page.get_by_test_id("root-positive-radius")).to_have_value("24")
                expect(self.circle("p-main")).to_have_class("section-circle is-selected")
                self.assertEqual(svg.inner_html(), solved_before)
        self.import_json(json.dumps(bilateral_case()))
        expect(self.page.get_by_test_id("root-positive-radius")).to_have_value("20")
        self.status_is("solved")

    def test_failure_keeps_only_the_last_successful_geometry(self):
        self.open_page()
        self.add_nested_child()
        self.status_is("solved")
        circle = self.circle("p-main")
        before = self.center("p-main")
        self.select_section("p-main")
        self.field("p-main", "radius").fill("2")
        self.status_is("unsolved")
        expect(self.page.get_by_test_id("diagnostics")).to_contain_text("FixedBoundaryInsufficient")
        expect(self.page.get_by_test_id("layout-positive")).to_have_attribute("data-stale", "true")
        expect(circle).to_have_attribute("r", "8")
        self.assertEqual(self.center("p-main"), before)
        expect(self.field("p-main", "radius")).to_have_value("2")
        self.add_section()
        self.tree_node("p-2").click()
        expect(self.circle("p-2")).to_have_count(0)
        expect(self.page.locator("[data-solved-details]")).to_have_count(0)
        self.select_section("p-main")
        self.field("p-main", "radius").fill("8")
        self.status_is("solved")
        expect(self.page.get_by_test_id("layout-positive")).to_have_attribute("data-stale", "false")
        expect(self.circle("p-2")).to_have_count(1)

    def test_initial_solver_failure_draws_nothing(self):
        self.wrap_solver("""
                export default async function () {
                  const real = await createRealSolver();
                  return {solveCase(input) {
                    const draft = structuredClone(input);
                    draft.sections.push({...draft.sections[0], id: 'oversized-child',
                      parentId: 'p-main', shape: {kind: 'circular', radius: 9}});
                    return real.solveCase(draft);
                  }};
                }
            """)
        self.page.goto(self.origin + "/index.html")
        self.status_is("unsolved")
        expect(self.page.get_by_test_id("diagnostics")).to_contain_text("FixedBoundaryInsufficient")
        expect(self.page.locator("svg circle")).to_have_count(0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--directory", type=Path, default=Path("build-wasm/interactive"))
    parser.add_argument("--phase", choices=["editor", "integration"])
    args = parser.parse_args()
    DIRECTORY = args.directory.resolve()
    names = {"editor": "EditorTest", "integration": "IntegrationTest"}
    unittest.main(argv=[__file__] + ([names[args.phase]] if args.phase else []), verbosity=2)
