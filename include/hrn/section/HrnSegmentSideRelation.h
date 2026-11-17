#pragma once
#include "hrn/section/HrnSegment.h"
namespace hrn {
struct HrnSegmentSideRelation {
  HrnSegment* positive;
  HrnSegment* negative;
  bool same_position;
  bool valid() const;
};
}  // namespace hrn
