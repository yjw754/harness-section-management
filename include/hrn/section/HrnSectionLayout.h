#pragma once
#include "hrn/section/HrnSectionTypes.h"
#include <cstddef>
#include <memory>
namespace hrn {
struct HrnLayoutSolveSettings {
 unsigned algorithm_version;
 size_t max_iterations;
 double convergence_tolerance;
 HrnLayoutSolveSettings(unsigned version=1,size_t iterations=1000,double tolerance=1e-9) : algorithm_version(version),max_iterations(iterations),convergence_tolerance(tolerance) {}
 bool valid() const;
};
inline bool operator==(const HrnLayoutSolveSettings& a,const HrnLayoutSolveSettings& b) { return a.algorithm_version==b.algorithm_version&&a.max_iterations==b.max_iterations&&a.convergence_tolerance==b.convergence_tolerance; }
class HrnSectionLayout {
public:
 virtual ~HrnSectionLayout() {}
 virtual ZeHrnSectionLayoutType type() const=0;
 virtual double spacingTolerance() const=0;
 virtual const HrnLayoutSolveSettings& solveSettings() const=0;
 virtual std::unique_ptr<HrnSectionLayout> clone() const=0;
};
class HrnRuleBasedLayout : public HrnSectionLayout {
public:
 HrnRuleBasedLayout() : spacing_(0),settings_() {}
 bool setSpacingTolerance(double spacing);
 bool setSolveSettings(const HrnLayoutSolveSettings& settings);
 ZeHrnSectionLayoutType type() const override { return ZeHrnSectionLayoutType::RuleBased; }
 double spacingTolerance() const override { return spacing_; }
 const HrnLayoutSolveSettings& solveSettings() const override { return settings_; }
 std::unique_ptr<HrnSectionLayout> clone() const override;
private:
 double spacing_;
 HrnLayoutSolveSettings settings_;
};
}
