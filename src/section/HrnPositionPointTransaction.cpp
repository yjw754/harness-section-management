#include "hrn/section/HrnPositionPoint.h"

#include <algorithm>
#include <set>
#include <utility>

#include "hrn/section/HrnPositionPointSolver.h"

namespace hrn {
namespace {
bool validSide(ZeHrnSide side) {
  return side == ZeHrnSide::Positive || side == ZeHrnSide::Negative;
}
HrnSectionTransactionResult invalid(HrnSectionTarget target) {
  HrnSectionTransactionResult result;
  result.primary_reason = HrnSectionFailureReason::InvalidInput;
  result.diagnostics.push_back(makeFailureDiagnostic(target,
      HrnSectionFailureStage::InputValidation, result.primary_reason, 0, 0));
  return result;
}
enum class EditKind { ReplaceSection, ReplaceRoot, Transform, Append, Remove, Reorder, Move };
struct Edit {
  EditKind kind = EditKind::Transform;
  ZeHrnSide side = ZeHrnSide::Positive;
  ZeHrnSide target_side = ZeHrnSide::Negative;
  const HrnSegment* segment = nullptr;
  std::unique_ptr<HrnSectionData> section;
  Matrix4D transform;
  std::vector<HrnSegment*> order;
};
}  // namespace

std::unique_ptr<HrnPositionPoint> HrnPositionPoint::createSolved(
    const HrnPositionPointInput& input,
    const std::vector<HrnSegmentSideRelation>& relations,
    const HrnPositionPointSolver& solver, HrnSectionTransactionResult& status) {
  for (const auto& relation : relations) {
    size_t positive = 0, negative = 0;
    if (!relation.valid() ||
        !input.findSectionIndex(ZeHrnSide::Positive, *relation.positive, positive) ||
        !input.findSectionIndex(ZeHrnSide::Negative, *relation.negative, negative)) {
      status = invalid(HrnSectionTarget::positionPoint());
      return std::unique_ptr<HrnPositionPoint>();
    }
  }
  HrnCircularHierarchyResult solved = solver.solveCircular(input, relations);
  status = std::move(solved.status);
  if (!status.committed) return std::unique_ptr<HrnPositionPoint>();
  for (int k = 0; k < 2; ++k) {
    const auto& side = solved.sides[k];
    if (!side.root || side.segments.size() != side.sections.size() ||
        side.segments.size() != side.locations.size()) {
      status = invalid(HrnSectionTarget::positionPoint());
      status.primary_reason = HrnSectionFailureReason::InternalError;
      status.diagnostics.front().reason = HrnSectionFailureReason::InternalError;
      status.diagnostics.front().stage = HrnSectionFailureStage::Commit;
      return std::unique_ptr<HrnPositionPoint>();
    }
  }
  std::unique_ptr<HrnPositionPoint> result(new HrnPositionPoint(input.positionTransform(),
      std::move(solved.sides[0].root), std::move(solved.sides[1].root)));
  for (int k = 0; k < 2; ++k) {
    result->sides_[k].segments = std::move(solved.sides[k].segments);
    result->sides_[k].sections = std::move(solved.sides[k].sections);
    result->sides_[k].locations = std::move(solved.sides[k].locations);
  }
  result->relations_ = relations;
  return result;
}

class HrnPositionPointTransaction {
 public:
  static HrnSectionTransactionResult run(HrnPositionPoint& point, Edit edit,
                                         const HrnPositionPointSolver& solver) {
    if (edit.section) {
      bool shared_shape = false, shared_layout = false;
      for (const HrnPositionPoint::SideData& side : point.sides_) {
        const auto inspect = [&](const HrnSectionData* owned) -> bool {
          if (edit.section.get() == owned) return true;
          shared_shape = shared_shape || &edit.section->shape() == &owned->shape();
          shared_layout = shared_layout || &edit.section->layout() == &owned->layout();
          return false;
        };
        if (inspect(side.root.get())) {
          edit.section.release();
          return invalid(HrnSectionTarget::positionPoint());
        }
        for (const auto& owned : side.sections)
          if (inspect(owned.get())) {
            edit.section.release();
            return invalid(HrnSectionTarget::positionPoint());
          }
      }
      if (shared_shape || shared_layout) {
        if (shared_shape) edit.section->shape_.release();
        if (shared_layout) edit.section->layout_.release();
        return invalid(HrnSectionTarget::positionPoint());
      }
    }
    if (edit.kind != EditKind::Transform && !validSide(edit.side))
      return invalid(HrnSectionTarget::positionPoint());
    if (edit.kind == EditKind::Move &&
        (!validSide(edit.target_side) || edit.side == edit.target_side))
      return invalid(HrnSectionTarget::positionPoint());
    if ((edit.kind == EditKind::Append || edit.kind == EditKind::ReplaceSection ||
         edit.kind == EditKind::ReplaceRoot) &&
        (!edit.section || !HrnPositionPoint::validateSectionData(*edit.section)))
      return invalid(HrnSectionTarget::positionPoint());
    if (edit.kind == EditKind::Transform &&
        !isRigidTransform(edit.transform, 1e-9))
      return invalid(HrnSectionTarget::positionPoint());
    if ((edit.kind == EditKind::Append || edit.kind == EditKind::Remove ||
         edit.kind == EditKind::ReplaceSection || edit.kind == EditKind::Move) &&
        !edit.segment) return invalid(HrnSectionTarget::positionPoint());

    std::vector<HrnSegment*> order[2] = {point.sides_[0].segments,
                                          point.sides_[1].segments};
    const int source = edit.side == ZeHrnSide::Positive ? 0 : 1;
    const int destination = edit.target_side == ZeHrnSide::Positive ? 0 : 1;
    if (edit.kind == EditKind::Append) {
      if (std::find(order[source].begin(), order[source].end(), edit.segment) !=
          order[source].end()) return invalid(HrnSectionTarget::positionPoint());
      order[source].push_back(const_cast<HrnSegment*>(edit.segment));
    } else if (edit.kind == EditKind::Remove || edit.kind == EditKind::Move) {
      const auto found = std::find(order[source].begin(), order[source].end(), edit.segment);
      if (found == order[source].end()) return invalid(HrnSectionTarget::positionPoint());
      order[source].erase(found);
      if (edit.kind == EditKind::Move) {
        if (std::find(order[destination].begin(), order[destination].end(), edit.segment) !=
            order[destination].end()) return invalid(HrnSectionTarget::positionPoint());
        order[destination].push_back(const_cast<HrnSegment*>(edit.segment));
      }
    } else if (edit.kind == EditKind::ReplaceSection) {
      if (std::find(order[source].begin(), order[source].end(), edit.segment) ==
          order[source].end()) return invalid(HrnSectionTarget::positionPoint());
    } else if (edit.kind == EditKind::Reorder) {
      if (edit.order.size() != order[source].size() ||
          std::set<HrnSegment*>(edit.order.begin(), edit.order.end()) !=
          std::set<HrnSegment*>(order[source].begin(), order[source].end()))
        return invalid(HrnSectionTarget::positionPoint());
      order[source] = edit.order;
    }

    const Matrix4D& transform = edit.kind == EditKind::Transform ? edit.transform :
                                 point.transform_;
    std::unique_ptr<HrnSectionData> positive_root =
        edit.kind == EditKind::ReplaceRoot && source == 0 ? std::move(edit.section) :
        point.sides_[0].root->clone();
    std::unique_ptr<HrnSectionData> negative_root =
        edit.kind == EditKind::ReplaceRoot && source == 1 ? std::move(edit.section) :
        point.sides_[1].root->clone();
    auto candidate = HrnPositionPointInput::create(transform, std::move(positive_root),
                                                    std::move(negative_root));
    if (!candidate) return invalid(HrnSectionTarget::positionPoint());
    for (int k = 0; k < 2; ++k) {
      const ZeHrnSide side = k == 0 ? ZeHrnSide::Positive : ZeHrnSide::Negative;
      for (HrnSegment* segment : order[k]) {
        std::unique_ptr<HrnSectionData> section;
        if (edit.kind == EditKind::Append && k == source && segment == edit.segment)
          section = std::move(edit.section);
        else if (edit.kind == EditKind::ReplaceSection && k == source &&
                 segment == edit.segment)
          section = std::move(edit.section);
        else {
          size_t previous = 0;
          if (point.findSectionIndex(side, *segment, previous))
            section = point.sides_[k].sections[previous]->clone();
          else if (edit.kind == EditKind::Move && k == destination &&
                   point.findSectionIndex(edit.side, *segment, previous))
            section = point.sides_[source].sections[previous]->clone();
        }
        if (!section || !candidate->append(side, *segment, std::move(section)))
          return invalid(HrnSectionTarget::positionPoint());
      }
    }
    HrnSectionTransactionResult status;
    auto replacement = HrnPositionPoint::createSolved(*candidate, point.relations_, solver,
                                                      status);
    if (!replacement) return status;
    point.transform_ = replacement->transform_;
    for (int k = 0; k < 2; ++k) point.sides_[k] = std::move(replacement->sides_[k]);
    point.relations_ = std::move(replacement->relations_);
    return status;
  }
};

HrnSectionTransactionResult HrnPositionPoint::replaceSectionData(
    ZeHrnSide side, const HrnSegment& segment, std::unique_ptr<HrnSectionData> section,
    const HrnPositionPointSolver& solver) {
  Edit edit;
  edit.kind = EditKind::ReplaceSection;
  edit.side = side;
  edit.segment = &segment;
  edit.section = std::move(section);
  return HrnPositionPointTransaction::run(*this, std::move(edit), solver);
}
HrnSectionTransactionResult HrnPositionPoint::replaceSideRootSectionData(
    ZeHrnSide side, std::unique_ptr<HrnSectionData> section,
    const HrnPositionPointSolver& solver) {
  Edit edit;
  edit.kind = EditKind::ReplaceRoot;
  edit.side = side;
  edit.section = std::move(section);
  return HrnPositionPointTransaction::run(*this, std::move(edit), solver);
}
HrnSectionTransactionResult HrnPositionPoint::replacePositionTransform(
    const Matrix4D& transform, const HrnPositionPointSolver& solver) {
  Edit edit;
  edit.kind = EditKind::Transform;
  edit.transform = transform;
  return HrnPositionPointTransaction::run(*this, std::move(edit), solver);
}
HrnSectionTransactionResult HrnPositionPoint::appendSectionItem(
    ZeHrnSide side, HrnSegment& segment, std::unique_ptr<HrnSectionData> section,
    const HrnPositionPointSolver& solver) {
  Edit edit;
  edit.kind = EditKind::Append;
  edit.side = side;
  edit.segment = &segment;
  edit.section = std::move(section);
  return HrnPositionPointTransaction::run(*this, std::move(edit), solver);
}
HrnSectionTransactionResult HrnPositionPoint::removeSectionItem(
    ZeHrnSide side, const HrnSegment& segment, const HrnPositionPointSolver& solver) {
  Edit edit;
  edit.kind = EditKind::Remove;
  edit.side = side;
  edit.segment = &segment;
  return HrnPositionPointTransaction::run(*this, std::move(edit), solver);
}
HrnSectionTransactionResult HrnPositionPoint::reorderSectionItems(
    ZeHrnSide side, const std::vector<HrnSegment*>& order,
    const HrnPositionPointSolver& solver) {
  Edit edit;
  edit.kind = EditKind::Reorder;
  edit.side = side;
  edit.order = order;
  return HrnPositionPointTransaction::run(*this, std::move(edit), solver);
}
HrnSectionTransactionResult HrnPositionPoint::moveSectionItem(
    ZeHrnSide source_side, ZeHrnSide target_side, const HrnSegment& segment,
    const HrnPositionPointSolver& solver) {
  Edit edit;
  edit.kind = EditKind::Move;
  edit.side = source_side;
  edit.target_side = target_side;
  edit.segment = &segment;
  return HrnPositionPointTransaction::run(*this, std::move(edit), solver);
}
}  // namespace hrn
