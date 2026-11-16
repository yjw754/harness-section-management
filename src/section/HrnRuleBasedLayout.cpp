#include "hrn/section/HrnSectionLayout.h"
#include <cmath>
namespace hrn {
bool HrnLayoutSolveSettings::valid() const { return algorithm_version>0&&max_iterations>0&&std::isfinite(convergence_tolerance)&&convergence_tolerance>0; }
bool HrnRuleBasedLayout::setSpacingTolerance(double v) { if(!std::isfinite(v)||v<0) return false; spacing_=v; return true; }
bool HrnRuleBasedLayout::setSolveSettings(const HrnLayoutSolveSettings& v) { if(!v.valid()) return false; settings_=v; return true; }
std::unique_ptr<HrnSectionLayout> HrnRuleBasedLayout::clone() const { return std::unique_ptr<HrnSectionLayout>(new HrnRuleBasedLayout(*this)); }
}
