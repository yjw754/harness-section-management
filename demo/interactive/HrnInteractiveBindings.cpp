#include "HrnInteractiveCase.h"

#include <cmath>
#include <emscripten/bind.h>
#include <utility>

namespace {
using emscripten::val;
using namespace hrn;

constexpr const char* kSides[] = {"positive", "negative"};

struct PlacementHandle {
  std::shared_ptr<const HrnInteractivePlacementSnapshot> snapshot;
};

struct CaseReader {
  std::vector<HrnInteractiveFieldError> errors;

  bool fail(const std::string& path, const char* message) {
    errors.push_back({path, message});
    return false;
  }

  bool object(const val& value, const std::string& path) {
    return (!value.isNull() && value.typeOf().as<std::string>() == "object" &&
            !val::global("Array").call<bool>("isArray", value)) || fail(path, "必须为对象");
  }

  bool array(const val& value, const std::string& path) {
    return val::global("Array").call<bool>("isArray", value) || fail(path, "必须为数组");
  }

  bool number(const val& value, const std::string& path, double& out) {
    if (value.typeOf().as<std::string>() != "number") return fail(path, "必须为数字");
    out = value.as<double>();
    return std::isfinite(out) || fail(path, "必须为有限数字");
  }

  bool text(const val& value, const std::string& path, std::string& out) {
    if (value.typeOf().as<std::string>() != "string") return fail(path, "必须为字符串");
    out = value.as<std::string>();
    return true;
  }

  bool boolean(const val& value, const std::string& path, bool& out) {
    if (value.typeOf().as<std::string>() != "boolean") return fail(path, "必须为布尔值");
    out = value.as<bool>();
    return true;
  }

  bool options(const val& value, const std::string& path, HrnInteractiveSectionOptions& out) {
    if (!object(value, path)) return false;
    const val shape = value["shape"];
    if (!object(shape, path + ".shape") || !text(shape["kind"], path + ".shape.kind", out.shape.kind))
      return false;
    if (out.shape.kind != "circular") return fail(path + ".shape.kind", "当前仅支持圆形截面");
    return number(shape["radius"], path + ".shape.radius", out.shape.radius) &&
           number(value["rotation"], path + ".rotation", out.rotation) &&
           number(value["spacing"], path + ".spacing", out.spacing) &&
           boolean(value["autoResize"], path + ".autoResize", out.auto_resize);
  }

  bool read(const val& value, HrnInteractiveCaseDefinition& out) {
    double version = 0;
    if (!object(value, "$") || !number(value["schemaVersion"], "schemaVersion", version)) return false;
    if (version != 1) return fail("schemaVersion", "仅支持版本 1");
    if (!number(value["modelingTolerance"], "modelingTolerance", out.modeling_tolerance)) return false;
    const val transform = value["positionTransform"];
    if (!array(transform, "positionTransform")) return false;
    if (transform["length"].as<unsigned>() != 16)
      return fail("positionTransform", "需要 16 个矩阵元素");
    for (unsigned i = 0; i < 16; ++i) {
      if (!number(transform[i], "positionTransform[" + std::to_string(i) + "]",
                  out.position_transform.m[i / 4][i % 4])) return false;
    }
    const val roots = value["roots"];
    if (!object(roots, "roots")) return false;
    for (size_t i = 0; i < 2; ++i) {
      if (!options(roots[kSides[i]], std::string("roots.") + kSides[i], out.roots[i])) return false;
    }
    const val sections = value["sections"];
    if (!array(sections, "sections")) return false;
    for (unsigned i = 0, count = sections["length"].as<unsigned>(); i < count; ++i) {
      const std::string path = "sections[" + std::to_string(i) + "]";
      const val entry = sections[i];
      HrnInteractiveSectionDefinition section;
      if (!options(entry, path, section.options) || !text(entry["id"], path + ".id", section.id) ||
          !text(entry["label"], path + ".label", section.label) ||
          !text(entry["side"], path + ".side", section.side)) return false;
      const val parent = entry["parentId"];
      if (!parent.isNull()) {
        if (!text(parent, path + ".parentId", section.parent_id)) return false;
        if (section.parent_id.empty()) return fail(path + ".parentId", "需要非空 ID 或 null");
      }
      out.sections.push_back(std::move(section));
    }
    const val relations = value["relations"];
    if (!array(relations, "relations")) return false;
    for (unsigned i = 0, count = relations["length"].as<unsigned>(); i < count; ++i) {
      const std::string path = "relations[" + std::to_string(i) + "]";
      const val entry = relations[i];
      HrnInteractiveRelation relation;
      if (!object(entry, path) || !text(entry["positiveId"], path + ".positiveId", relation.positive_id) ||
          !text(entry["negativeId"], path + ".negativeId", relation.negative_id) ||
          !boolean(entry["samePosition"], path + ".samePosition", relation.same_position)) return false;
      out.relations.push_back(std::move(relation));
    }
    return true;
  }

  bool readRotation(const val& value, HrnInteractiveRotationBaseline& out) {
    const std::string path = "rotationBaseline";
    if (!object(value, path) ||
        !boolean(value["preferNegative"], path + ".preferNegative", out.prefer_negative)) return false;
    const val fallback = value["fallbackDelta"];
    if (!fallback.isUndefined() && !number(fallback, path + ".fallbackDelta", out.fallback_delta))
      return false;
    const val placement = value["placement"];
    if (placement.isUndefined() || placement.isNull())
      return fail(path + ".placement", "需要求解器生成的布局快照");
    const auto handle = placement.as<std::shared_ptr<PlacementHandle>>();
    if (!handle || !handle->snapshot) return fail(path + ".placement", "布局快照不可用");
    out.placement = handle->snapshot;
    return true;
  }
};

val shapeObject(const HrnInteractiveShape& shape) {
  val result = val::object();
  result.set("kind", shape.kind);
  result.set("radius", shape.radius);
  return result;
}

val resultObject(const HrnInteractiveCaseResult& result) {
  val out = val::object();
  out.set("valid", result.valid);
  out.set("solved", result.solved);
  out.set("usedGlobalRotation", result.used_global_rotation);
  out.set("frameRotation", result.frame_rotation);
  if (result.placement) {
    val descriptor = val::object();
    descriptor.set("value", val(std::make_shared<PlacementHandle>(PlacementHandle{result.placement})));
    val::global("Object").call<val>("defineProperty", out, std::string("placement"), descriptor);
  }
  out.set("status", std::string(result.solved ? "solved" : result.valid ? "unsolved" : "invalid"));
  val errors = val::array(), diagnostics = val::array(), roots = val::object();
  val nodes = val::array(), relations = val::array();
  for (const auto& error : result.errors) {
    val entry = val::object();
    entry.set("path", error.path);
    entry.set("message", error.message);
    errors.call<void>("push", entry);
  }
  for (const auto& diagnostic : result.diagnostics) diagnostics.call<void>("push", diagnostic);
  if (result.valid) {
    val counters = val::object();
    for (size_t i = 0; i < 2; ++i) {
      const auto& root = result.roots[i];
      val entry = val::object(), counter = val::object();
      entry.set("inputShape", shapeObject(root.input_shape));
      if (result.solved) {
        entry.set("solvedShape", shapeObject(root.solved_shape));
        entry.set("referenceRotation", root.reference_rotation);
      }
      roots.set(kSides[i], entry);
      counter.set("candidateCount", static_cast<double>(root.candidate_count));
      counter.set("iterationsUsed", static_cast<double>(root.iterations_used));
      counters.set(kSides[i], counter);
    }
    out.set("counters", counters);
  }
  for (const auto& node : result.nodes) {
    val entry = val::object();
    entry.set("id", node.id);
    entry.set("label", node.label);
    entry.set("side", node.side);
    entry.set("parentId", node.parent_id.empty() ? val::null() : val(node.parent_id));
    entry.set("depth", static_cast<double>(node.depth));
    entry.set("inputShape", shapeObject(node.input_shape));
    entry.set("inputRotation", node.input_rotation);
    if (node.has_geometry) {
      val center = val::object();
      center.set("x", node.center.x);
      center.set("y", node.center.y);
      entry.set("center", center);
      val reference = val::object();
      reference.set("x", node.reference_center.x);
      reference.set("y", node.reference_center.y);
      entry.set("referenceCenter", reference);
      entry.set("solvedShape", shapeObject(node.solved_shape));
      entry.set("solvedRotation", node.solved_rotation);
    }
    nodes.call<void>("push", entry);
  }
  for (const auto& relation : result.relations) {
    val entry = val::object();
    entry.set("positiveId", relation.positive_id);
    entry.set("negativeId", relation.negative_id);
    entry.set("samePosition", relation.same_position);
    relations.call<void>("push", entry);
  }
  out.set("errors", errors);
  out.set("diagnostics", diagnostics);
  out.set("roots", roots);
  out.set("nodes", nodes);
  out.set("relations", relations);
  return out;
}

val solve(const val& input, const val& baseline) {
  CaseReader reader;
  HrnInteractiveCaseDefinition definition;
  HrnInteractiveRotationBaseline rotation;
  const bool has_baseline = !baseline.isUndefined();
  if (reader.read(input, definition) && (!has_baseline || reader.readRotation(baseline, rotation)))
    return resultObject(solveInteractiveCase(definition, has_baseline ? &rotation : nullptr));
  HrnInteractiveCaseResult invalid;
  invalid.errors = std::move(reader.errors);
  return resultObject(invalid);
}

val solveCase(const val& input) { return solve(input, val::undefined()); }
val solveRotation(const val& input, const val& baseline) { return solve(input, baseline); }
}  // namespace

EMSCRIPTEN_BINDINGS(hrn_interactive_debugger) {
  emscripten::class_<PlacementHandle>("PlacementHandle")
      .smart_ptr<std::shared_ptr<PlacementHandle>>("PlacementHandlePtr");
  emscripten::function("solveCase", &solveCase);
  emscripten::function("solveRotation", &solveRotation);
}
