#pragma once

#include <cstddef>
#include <vector>

#include "hrn/section/HrnSegment.h"
#include "hrn/section/HrnSectionTypes.h"

namespace hrn {
enum class HrnSectionFailureReason {
  None, InvalidInput, InvalidTopology, FixedBoundaryInsufficient,
  SizeUpdateConstraintConflict, SamePositionConstraintConflict, ConstraintInfeasible,
  NoSolutionFound, IterationBudgetExceeded, NonConvergent,
  GeometryValidationFailed, InternalError
};
enum class HrnSectionFailureStage {
  InputValidation, TopologyValidation, LayoutSolve, ParentSizeUpdate,
  GeometryValidation, Commit
};
enum class HrnSectionTargetKind { PositionPoint, SideRootSection, SegmentSection };
struct HrnSectionTarget {
  HrnSectionTargetKind kind = HrnSectionTargetKind::PositionPoint;
  bool has_side = false;
  ZeHrnSide side = ZeHrnSide::Positive;
  const HrnSegment* segment = nullptr;
  static HrnSectionTarget positionPoint();
  static HrnSectionTarget sideRoot(ZeHrnSide side);
  static HrnSectionTarget segmentSection(ZeHrnSide side, const HrnSegment& segment);
};
enum class HrnSectionConflictKind {
  OutsideParent, Overlap, ClearanceInsufficient, FrozenDimensionInsufficient,
  SamePositionIncompatible, InvalidGeometry
};
struct HrnSectionConflict {
  HrnSectionConflictKind kind = HrnSectionConflictKind::InvalidGeometry;
  HrnSectionTarget lhs;
  bool has_rhs = false;
  HrnSectionTarget rhs;
  bool has_measured_value = false;
  double measured_value = 0;
  bool has_required_value = false;
  double required_value = 0;
  bool has_lhs_witness = false;
  Point2D lhs_witness{0, 0};
  bool has_rhs_witness = false;
  Point2D rhs_witness{0, 0};
};
enum class HrnSectionAdjustmentKind {
  IncreaseParentDimension, EnableAutoResize, ChangeSizeUpdateMode,
  ReduceSpacing, ReduceChildDimension, ChangeRotation, IncreaseIterationBudget,
  ChangeLayoutStrategy, RemoveSamePositionConstraint
};
enum class HrnSectionAdjustmentCertainty {
  SufficientForCurrentPlacement, DeterministicCandidate, HeuristicSuggestion
};
enum class HrnSectionParameterKey {
  Radius, MajorRadius, MinorRadius, LongSide, ShortSide, AutoResizeEnabled,
  SizeUpdateMode, SpacingTolerance, Rotation, MaxIterations, LayoutType,
  SamePositionConstraint
};
enum class HrnSectionAdjustmentValueType { Number, Boolean, Count, SizeUpdateMode, LayoutType };
struct HrnSectionAdjustmentValue {
  HrnSectionAdjustmentValueType value_type = HrnSectionAdjustmentValueType::Number;
  double number_value = 0;
  bool bool_value = false;
  size_t count_value = 0;
  HrnSectionSizeUpdateMode size_update_mode = HrnSectionSizeUpdateMode::KeepRatioConstant;
  ZeHrnSectionLayoutType layout_type = ZeHrnSectionLayoutType::RuleBased;
  static HrnSectionAdjustmentValue number(double value);
  static HrnSectionAdjustmentValue boolean(bool value);
  static HrnSectionAdjustmentValue count(size_t value);
  static HrnSectionAdjustmentValue mode(HrnSectionSizeUpdateMode value);
};
struct HrnSectionAdjustment {
  HrnSectionAdjustmentKind kind = HrnSectionAdjustmentKind::IncreaseParentDimension;
  HrnSectionTarget target;
  HrnSectionParameterKey parameter = HrnSectionParameterKey::Radius;
  bool has_current_value = false;
  HrnSectionAdjustmentValue current_value;
  bool has_suggested_value = false;
  HrnSectionAdjustmentValue suggested_value;
  HrnSectionAdjustmentCertainty certainty = HrnSectionAdjustmentCertainty::HeuristicSuggestion;
  std::vector<HrnSectionTarget> related_targets;
};
struct HrnSectionSolveDiagnostic {
  HrnSectionTarget target;
  HrnSectionFailureStage stage = HrnSectionFailureStage::InputValidation;
  HrnSectionFailureReason reason = HrnSectionFailureReason::None;
  std::vector<HrnSectionConflict> conflicts;
  std::vector<HrnSectionAdjustment> adjustments;
  unsigned algorithm_version = 0;
  size_t iterations_used = 0;
};
struct HrnSectionTransactionResult {
  bool committed = false;
  HrnSectionFailureReason primary_reason = HrnSectionFailureReason::None;
  std::vector<HrnSectionSolveDiagnostic> diagnostics;
};
HrnSectionSolveDiagnostic makeFailureDiagnostic(HrnSectionTarget target,
    HrnSectionFailureStage stage, HrnSectionFailureReason reason,
    unsigned algorithm_version, size_t iterations_used);
}  // namespace hrn
