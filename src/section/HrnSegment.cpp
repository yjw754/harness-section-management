#include "hrn/section/HrnSegment.h"
namespace hrn {
bool HrnSegment::attachTo(HrnSegment& candidate) {
  if (parent_ || &candidate == this) return false;
  for (HrnSegment* p = &candidate; p; p = p->parent_)
    if (p == this) return false;
  parent_ = &candidate;
  candidate.children_.push_back(this);
  return true;
}
}  // namespace hrn
