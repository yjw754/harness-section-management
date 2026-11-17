#pragma once
#include <vector>
namespace hrn {
class HrnSegment {
 public:
  HrnSegment() : parent_(0) {}
  HrnSegment* parent() const { return parent_; }
  const std::vector<HrnSegment*>& children() const { return children_; }
  bool attachTo(HrnSegment& parent);

 private:
  HrnSegment* parent_;
  std::vector<HrnSegment*> children_;
  HrnSegment(const HrnSegment&) = delete;
  HrnSegment& operator=(const HrnSegment&) = delete;
};
}  // namespace hrn
