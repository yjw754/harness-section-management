"""Exercise the generated Emscripten module in Chromium over loopback HTTP."""

import argparse
import json
import math
from collections import defaultdict
from contextlib import contextmanager
from functools import partial
from hashlib import sha256
from http.server import ThreadingHTTPServer
from itertools import combinations
from pathlib import Path
import shutil
import subprocess
import sys
from tempfile import TemporaryDirectory
from threading import Thread
import unittest
from urllib.parse import urlsplit

from playwright.sync_api import expect, sync_playwright


REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "demo/interactive"))
from serve_debugger import DebuggerHandler


class QuietHandler(DebuggerHandler):
    def log_message(self, *_):
        pass


@contextmanager
def serve(directory):
    handler = partial(QuietHandler, directory=str(directory))
    with ThreadingHTTPServer(("127.0.0.1", 0), handler) as server:
        thread = Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            yield f"http://127.0.0.1:{server.server_port}"
        finally:
            server.shutdown()
            thread.join()


def bilateral_case():
    def options(radius):
        return {"shape": {"kind": "circular", "radius": radius}, "rotation": 0,
                "spacing": 0, "autoResize": False}

    return {
        "schemaVersion": 1, "modelingTolerance": 1e-9,
        "positionTransform": [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1],
        "roots": {"positive": options(20), "negative": options(20)},
        "sections": [
            {"id": "p-main", "label": "P main", "side": "positive", "parentId": None, **options(8)},
            {"id": "n-main", "label": "N main", "side": "negative", "parentId": None, **options(8)},
        ],
        "relations": [{"positiveId": "p-main", "negativeId": "n-main", "samePosition": True}],
    }


class DebuggerWasmSmokeTest(unittest.TestCase):
    def test_rotation_cannot_reuse_placement_after_constraints_change(self):
        draft = bilateral_case()
        draft["relations"] = []
        draft["sections"].append({**draft["sections"][0], "id": "p-child", "parentId": "p-main",
                                  "shape": {"kind": "circular", "radius": 3}})
        with serve(DIRECTORY) as origin, sync_playwright() as playwright:
            browser = playwright.chromium.launch()
            page = browser.new_page()
            page.goto(origin)
            result = page.evaluate("""async draft => {
              const {default: createSolver} = await import(document.querySelector('[data-solver-module]').dataset.solverModule);
              const {solveCase, solveRotation} = await createSolver();
              const previous = solveCase(draft);
              if (!previous.solved) throw Error(previous.diagnostics.join('; '));
              const baseline = {
                placement: previous.placement,
                rootRotations: [0, 0],
                solvedRootRadii: ['positive', 'negative'].map(side => previous.roots[side].solvedShape.radius),
                nodes: previous.nodes, preferNegative: false, fallbackDelta: 90,
              };
              draft.sections[0].rotation = 90;
              draft.sections[0].spacing = 6;
              const changed = solveRotation(draft, baseline);
              const raw = solveRotation(draft, {...baseline, placement: undefined});
              draft.sections[0].spacing = 0;
              const rawValid = solveRotation(draft, {...baseline, placement: undefined});
              previous.placement?.delete();
              changed.placement?.delete();
              raw.placement?.delete();
              rawValid.placement?.delete();
              return {changed, raw, rawValid};
            }""", draft)
            browser.close()
        self.assertFalse(result["changed"]["solved"], result)
        self.assertFalse(result["raw"]["solved"], result)
        self.assertEqual(result["rawValid"]["status"], "invalid", result)

    def assert_rotation_geometry(self, draft, result, edited_id=None, delta=0):
        self.assertEqual(result["status"], "solved", (result["errors"], result["diagnostics"]))
        self.assertTrue(result["valid"] and result["solved"])
        self.assertEqual(result["relations"], draft["relations"])
        tolerance = draft["modelingTolerance"]
        sections = {entry["id"]: entry for entry in draft["sections"]}
        nodes = {node["id"]: node for node in result["nodes"]}
        self.assertEqual(nodes.keys(), sections.keys())
        self.assertEqual(len(result["nodes"]), len(sections))
        self.assertEqual(result["roots"].keys(), draft["roots"].keys())

        groups = {node_id: {node_id} for node_id in sections}
        for relation in draft["relations"]:
            if relation["samePosition"]:
                group = groups[relation["positiveId"]] | groups[relation["negativeId"]]
                for node_id in group:
                    groups[node_id] = group

        def radius(entry, solved):
            shape = solved["solvedShape"]
            self.assertEqual(shape["kind"], "circular")
            self.assertTrue(math.isfinite(shape["radius"]) and shape["radius"] > 0, shape)
            if not entry["autoResize"]:
                self.assertEqual(shape, entry["shape"], entry)
            return shape["radius"]

        centers, radii = {}, {}
        families = defaultdict(list)
        for node_id, entry in sections.items():
            node = nodes[node_id]
            self.assertEqual((node["side"], node["parentId"]), (entry["side"], entry["parentId"]))
            self.assertEqual(node["inputRotation"], entry["rotation"] + (delta if node_id == edited_id else 0))
            center = node["center"]["x"], node["center"]["y"]
            self.assertTrue(all(math.isfinite(coordinate) for coordinate in center), node_id)
            centers[node_id] = center
            radii[node_id] = radius(entry, node)
            families[entry["side"], entry["parentId"]].append(node_id)

        for node_id, group in groups.items():
            peer = min(group)
            self.assertLessEqual(math.dist(centers[node_id], centers[peer]), tolerance,
                                 f"{node_id} and {peer} must share a center")

        root_radii = {side: radius(entry, result["roots"][side]) for side, entry in draft["roots"].items()}
        for (side, parent), children in families.items():
            if parent is None:
                if len(children) == 1:  # A singleton bypasses the side Root boundary.
                    continue
                spacing = draft["roots"][side]["spacing"]
                parent_center, parent_radius = (0, 0), root_radii[side]
            else:
                spacing = sections[parent]["spacing"]
                parent_center, parent_radius = centers[parent], radii[parent]
            for child in children:
                extent = math.dist(centers[child], parent_center) + radii[child] + spacing
                self.assertLessEqual(extent, parent_radius + tolerance,
                                     f"{child} must fit within {parent or side + ' Root'}")
            for left, right in combinations(children, 2):
                if right not in groups[left]:
                    self.assertGreaterEqual(math.dist(centers[left], centers[right]) + tolerance,
                                            radii[left] + radii[right] + spacing,
                                            f"{left} and {right} must have clearance {spacing}")
        return centers

    def test_independent_rotation_matrix(self):
        cases = REPO / "docs/verification/cases"
        names = ["N_1.json", "1_N.json", "N_N_same_position.json", "N_1_mixed_radii.json",
                 "partial_nested.json", "same_side_rot.json", "stage2-singleton-follows.json",
                 "rotation-deep-ordinary.json", "rotation-nested-transitive.json",
                 "rotation-mixed-constraints.json", "rotation-empty-side.json"]
        deltas = [45, -90, 180, 360]
        drafts = {name: json.loads((cases / name).read_text()) for name in names}
        no_relations = "rotation-deep-ordinary (no relations)"
        drafts[no_relations] = {**drafts["rotation-deep-ordinary.json"], "relations": []}
        with serve(DIRECTORY) as origin, sync_playwright() as playwright:
            browser = playwright.chromium.launch()
            try:
                page = browser.new_page()
                page.goto(origin)
                results = page.evaluate("""async ({drafts, deltas}) => {
                  const moduleUrl = document.querySelector('[data-solver-module]').dataset.solverModule;
                  const {default: createSolver} = await import(moduleUrl);
                  const {solveCase, solveRotation} = await createSolver();
                  const sides = ['positive', 'negative'];
                  return drafts.map(draft => {
                    const previous = solveCase(draft);
                    const edits = [];
                    if (!previous.solved) return {previous, edits};
                    const targets = [...sides.map(side => ({side, id: null})), ...draft.sections];
                    for (const {side, id} of targets) {
                      for (const delta of deltas) {
                        const edited = structuredClone(draft);
                        const target = id === null ? edited.roots[side] : edited.sections.find(node => node.id === id);
                        target.rotation += delta;
                        const result = solveRotation(edited, {
                          placement: previous.placement,
                          preferNegative: side === 'negative',
                          fallbackDelta: delta,
                        });
                        result.placement?.delete();
                        edits.push({side, id, delta, result});
                      }
                    }
                    previous.placement?.delete();
                    return {previous, edits};
                  });
                }""", {"drafts": list(drafts.values()), "deltas": deltas})
            finally:
                browser.close()

        for (name, draft), case in zip(drafts.items(), results):
            with self.subTest(case=name):
                before = self.assert_rotation_geometry(draft, case["previous"])
                tolerance = draft["modelingTolerance"]
                occupied_sides = {entry["side"] for entry in draft["sections"]}
                for edit in case["edits"]:
                    node_id, side, delta = edit["id"], edit["side"], edit["delta"]
                    result = edit["result"]
                    with self.subTest(target=node_id or f"{side} Root", delta=delta):
                        actual = self.assert_rotation_geometry(draft, result, node_id, delta)
                        fallback = result.get("usedGlobalRotation")
                        self.assertIs(type(fallback), bool, "WASM must report whether global rotation was used")
                        if (name, node_id, delta) == ("partial_nested.json", "p-3", 180):
                            self.assertTrue(fallback)
                        local_example = (name in ("rotation-deep-ordinary.json", no_relations)
                                         and (node_id, delta) == ("p-branch", -90))
                        empty_root = node_id is None and side not in occupied_sides
                        if local_example or empty_root:
                            self.assertFalse(fallback)

                        expected = None
                        if fallback:
                            angle = math.radians(delta)
                            c, s = math.cos(angle), math.sin(angle)
                            expected = {key: (c * x - s * y, s * x + c * y) for key, (x, y) in before.items()}
                            for node, prior in zip(result["nodes"], case["previous"]["nodes"]):
                                self.assertEqual(node["id"], prior["id"])
                                self.assertEqual(node["solvedShape"], prior["solvedShape"])
                                self.assertLessEqual(abs(math.remainder(
                                    node["solvedRotation"] - prior["solvedRotation"] - delta, 360)), tolerance)
                            for side in draft["roots"]:
                                self.assertEqual(result["roots"][side]["solvedShape"],
                                                 case["previous"]["roots"][side]["solvedShape"])
                        elif delta == 360 or empty_root:
                            expected = before
                        elif local_example:
                            expected = before.copy()
                            px, py = before["p-branch"]
                            for child in ("p-tip", "p-twig"):
                                x, y = before[child]
                                expected[child] = (px + (y - py), py - (x - px))
                        if expected is not None:
                            for key, center in expected.items():
                                self.assertLessEqual(math.dist(actual[key], center), tolerance,
                                                     f"{key} moved to {actual[key]}, expected {center}")

    def test_fingerprinted_assets_change_on_normal_reload(self):
        def open_and_check(page, url=None):
            responses = {}

            def record(response):
                path = urlsplit(response.url).path
                if path.endswith((".js", ".wasm", ".css")):
                    responses[path] = response

            page.on("response", record)
            response = page.goto(url) if url else page.reload()
            expect(page.get_by_test_id("status")).to_have_attribute("data-state", "solved")
            page.remove_listener("response", record)
            self.assertEqual(len(responses), 4, responses)
            for path, asset in responses.items():
                self.assertRegex(path, r"^/[^/]+\.[a-f0-9]{64}\.(js|wasm|css)$")
                self.assertIn("immutable", asset.headers.get("cache-control", ""))
                if url:  # Check fresh response bodies, not browser cache revalidations.
                    self.assertEqual(Path(path).name.rsplit(".", 2)[1],
                                     sha256(asset.body()).hexdigest())
            self.assertIn("no-store", response.headers.get("cache-control", ""))
            return set(responses)

        with sync_playwright() as playwright:
            browser = playwright.chromium.launch()
            with serve(DIRECTORY) as origin:
                open_and_check(browser.new_page(), origin)

            with TemporaryDirectory() as temporary:
                scratch = Path(temporary)
                raw = scratch / "linked"
                shutil.copytree(LINKED_DIRECTORY, raw)
                published = scratch / "interactive"

                def package():
                    subprocess.run(["cmake", f"-DSOURCE_DIR={REPO / 'demo/interactive'}",
                                    f"-DLINKED_DIR={raw}", f"-DOUTPUT_DIR={published}",
                                    "-P", str(REPO / "cmake/HrnPackageInteractive.cmake")], check=True)

                package()
                with serve(published) as origin:
                    page = browser.new_page()
                    before = open_and_check(page, origin)
                    entry_mtime = (published / "index.html").stat().st_mtime_ns
                    package()
                    self.assertEqual(open_and_check(page), before)
                    self.assertEqual((published / "index.html").stat().st_mtime_ns, entry_mtime)
                    with (raw / "hrn_interactive_solver.wasm").open("ab") as binary:
                        binary.write(b"\x00\x02\x01x")  # Append an empty, valid WASM custom section.
                    package()
                    after = open_and_check(page)
                    self.assertEqual({Path(path).suffix for path in after - before}, {".js", ".wasm"})
                    self.assertEqual(len(after - before), 2)
                    for path in before - after:
                        self.assertTrue(page.request.get(origin + path).ok)
                    entry = page.request.get(origin, headers={
                        "If-Modified-Since": "Fri, 01 Jan 2038 00:00:00 GMT"})
                    self.assertEqual(entry.status, 200)

    def test_deranged_case_recompacts_free_members(self):
        fixture = Path(__file__).resolve().parents[2] / "docs/verification/cases/deranged.json"
        draft = json.loads(fixture.read_text())
        mirror = json.loads(json.dumps(draft))
        for section in mirror["sections"]:
            section["side"] = "negative" if section["side"] == "positive" else "positive"
        for relation in mirror["relations"]:
            relation["positiveId"], relation["negativeId"] = relation["negativeId"], relation["positiveId"]
        with serve(DIRECTORY) as origin, sync_playwright() as playwright:
            browser = playwright.chromium.launch()
            try:
                page = browser.new_page()
                page.goto(origin)
                results = page.evaluate("""async (drafts) => {
                  const {default: createSolver} = await import(document.querySelector('[data-solver-module]').dataset.solverModule);
                  const {solveCase} = await createSolver();
                  return drafts.map(draft => solveCase(draft));
                }""", [draft, mirror])
            finally:
                browser.close()
        for side, result in zip(("positive", "negative"), results):
            with self.subTest(side=side):
                self.assertTrue(result["valid"] and result["solved"], result)
                nodes = {n["id"]: n for n in result["nodes"]}
                crowded = [n for n in result["nodes"] if n["side"] == side]
                self.assertLessEqual(max(math.hypot(n["center"]["x"], n["center"]["y"])
                                         + n["solvedShape"]["radius"] for n in crowded), 15)
                for index, a in enumerate(crowded):
                    for b in crowded[index + 1:]:
                        distance = math.hypot(a["center"]["x"] - b["center"]["x"],
                                              a["center"]["y"] - b["center"]["y"])
                        self.assertGreaterEqual(distance + 1e-9,
                                                a["solvedShape"]["radius"] + b["solvedShape"]["radius"])
                main = nodes["p-main"]["center"]
                for small, peer in (("p-4", "n-main"), ("p-5", "n-1")):
                    center = nodes[small]["center"]
                    self.assertAlmostEqual(math.hypot(main["x"] - center["x"],
                                                      main["y"] - center["y"]), 9, delta=1e-9)
                    self.assertAlmostEqual(center["x"], nodes[peer]["center"]["x"], delta=1e-9)
                    self.assertAlmostEqual(center["y"], nodes[peer]["center"]["y"], delta=1e-9)

    def test_imported_mapped_overlap_cases(self):
        cases = Path(__file__).resolve().parents[2] / "docs/verification/cases"
        names = ["N_1.json", "1_N.json", "N_1_mixed_radii.json", "N_N_same_position.json"]
        drafts = [json.loads((cases / name).read_text()) for name in names]
        with serve(DIRECTORY) as origin, sync_playwright() as playwright:
            browser = playwright.chromium.launch()
            try:
                page = browser.new_page()
                page.goto(origin)
                results = page.evaluate("""async (drafts) => {
                  const {default: createSolver} = await import(document.querySelector('[data-solver-module]').dataset.solverModule);
                  const {solveCase} = await createSolver();
                  return drafts.map(draft => solveCase(draft));
                }""", drafts)
            finally:
                browser.close()
        for name, draft, result in zip(names, drafts, results):
            with self.subTest(case=name):
                self.assertTrue(result["valid"] and result["solved"], result)
                self.assertEqual(result["status"], "solved")
                self.assertEqual(result["relations"], draft["relations"])
                self.assertEqual(result["diagnostics"], [])
                nodes = {node["id"]: node for node in result["nodes"]}
                self.assertEqual(len(nodes), len(draft["sections"]))
                for section in draft["sections"]:
                    node = nodes[section["id"]]
                    self.assertEqual(node["solvedShape"], section["shape"])
                    self.assertEqual(node["parentId"], section["parentId"])
                for relation in draft["relations"]:
                    if not relation["samePosition"]:
                        continue
                    p = nodes[relation["positiveId"]]["center"]
                    n = nodes[relation["negativeId"]]["center"]
                    self.assertAlmostEqual(p["x"], n["x"], delta=1e-9)
                    self.assertAlmostEqual(p["y"], n["y"], delta=1e-9)
                if name == "N_1_mixed_radii.json":
                    largest, other = nodes["p-main"], nodes["p-other"]
                    distance = math.hypot(largest["center"]["x"] - other["center"]["x"],
                                          largest["center"]["y"] - other["center"]["y"])
                    self.assertGreaterEqual(distance + 1e-9, 10)

    def test_wasm_boundary(self):
        with serve(DIRECTORY) as origin, sync_playwright() as playwright:
            browser = playwright.chromium.launch()
            try:
                page = browser.new_page()
                page.goto(origin)
                good, *invalid = page.evaluate("""async (draft) => {
                  const {default: createSolver} = await import(document.querySelector('[data-solver-module]').dataset.solverModule);
                  const {solveCase} = await createSolver();
                  const good = solveCase(draft);
                  return [good, ...['20', Infinity].map(radius => {
                    draft.roots.positive.shape.radius = radius;
                    return solveCase(draft);
                  })];
                }""", bilateral_case())
            finally:
                browser.close()

        self.assertTrue(good["valid"] and good["solved"], good)
        self.assertEqual(good["status"], "solved")
        self.assertEqual(good["relations"], [
            {"positiveId": "p-main", "negativeId": "n-main", "samePosition": True}])
        self.assertEqual({node["id"] for node in good["nodes"]}, {"p-main", "n-main"})
        for node in good["nodes"]:
            self.assertEqual(node["solvedShape"], {"kind": "circular", "radius": 8})
            self.assertAlmostEqual(node["center"]["x"], 0, delta=1e-9)
            self.assertAlmostEqual(node["center"]["y"], 0, delta=1e-9)
        for result in invalid:
            self.assertFalse(result["valid"] or result["solved"], result)
            self.assertEqual(result["status"], "invalid")
            self.assertIn("roots.positive.shape.radius", [error["path"] for error in result["errors"]])
            self.assertEqual(result["nodes"], [])
            self.assertEqual(result["roots"], {})


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--directory", type=Path, default=Path("build-wasm/interactive"))
    parser.add_argument("--linked-directory", type=Path)
    args, unittest_args = parser.parse_known_args()
    DIRECTORY = args.directory.resolve()
    LINKED_DIRECTORY = args.linked_directory or DIRECTORY.with_name("interactive-link")
    unittest.main(argv=[__file__, *unittest_args], verbosity=2)
