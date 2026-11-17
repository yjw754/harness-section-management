#include "hrn/section/HrnSectionDiagnostics.h"

namespace hrn {
HrnSectionTarget HrnSectionTarget::positionPoint() { return HrnSectionTarget(); }
HrnSectionTarget HrnSectionTarget::sideRoot(ZeHrnSide side) {
  HrnSectionTarget result;
  result.kind = HrnSectionTargetKind::SideRootSection;
  result.has_side = true;
  result.side = side;
  return result;
}
HrnSectionTarget HrnSectionTarget::segmentSection(ZeHrnSide side, const HrnSegment& segment) {
  HrnSectionTarget result = sideRoot(side);
  result.kind = HrnSectionTargetKind::SegmentSection;
  result.segment = &segment;
  return result;
}
HrnSectionAdjustmentValue HrnSectionAdjustmentValue::number(double value) {
  HrnSectionAdjustmentValue result;
  result.number_value = value;
  return result;
}
HrnSectionAdjustmentValue HrnSectionAdjustmentValue::boolean(bool value) {
  HrnSectionAdjustmentValue result;
  result.value_type = HrnSectionAdjustmentValueType::Boolean;
  result.bool_value = value;
  return result;
}
HrnSectionAdjustmentValue HrnSectionAdjustmentValue::count(size_t value) {
  HrnSectionAdjustmentValue result;
  result.value_type = HrnSectionAdjustmentValueType::Count;
  result.count_value = value;
  return result;
}
HrnSectionAdjustmentValue HrnSectionAdjustmentValue::mode(HrnSectionSizeUpdateMode value) {
  HrnSectionAdjustmentValue result;
  result.value_type = HrnSectionAdjustmentValueType::SizeUpdateMode;
  result.size_update_mode = value;
  return result;
}
HrnSectionSolveDiagnostic makeFailureDiagnostic(HrnSectionTarget target,
    HrnSectionFailureStage stage, HrnSectionFailureReason reason,
    unsigned algorithm_version, size_t iterations_used) {
  HrnSectionSolveDiagnostic result;
  result.target = target;
  result.stage = stage;
  result.reason = reason;
  result.algorithm_version = algorithm_version;
  result.iterations_used = iterations_used;
  return result;
}
}  // namespace hrn
