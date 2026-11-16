#include "hrn/section/HrnRootSectionDefaultProvider.h"
namespace hrn {
std::unique_ptr<HrnSectionData> HrnRootSectionDefaultProvider::create(ZeHrnSide) const {
 std::unique_ptr<HrnCircularSectionShape> shape=HrnCircularSectionShape::create(defaults_.radius,defaults_.modeling_tolerance);
 std::unique_ptr<HrnRuleBasedLayout> layout(new HrnRuleBasedLayout());
 if(!layout->setSpacingTolerance(defaults_.spacing)||!layout->setSolveSettings(defaults_.solve_settings)) return std::unique_ptr<HrnSectionData>();
 HrnSectionOptions o; o.rotation=defaults_.rotation; o.auto_resize_enabled=defaults_.auto_resize_enabled; o.size_update_mode=defaults_.size_update_mode;
 return HrnSectionData::create(std::move(shape),std::move(layout),o);
}
}
