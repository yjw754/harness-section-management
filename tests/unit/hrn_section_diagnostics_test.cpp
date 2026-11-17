#include <gtest/gtest.h>

#include "hrn/section/HrnSectionDiagnostics.h"

using namespace hrn;

TEST(HrnSectionDiagnostics, CarriesNodeSpecificConflictAndAdjustment) {
  const HrnSectionTarget parent = HrnSectionTarget::sideRoot(ZeHrnSide::Positive);
  HrnSectionSolveDiagnostic diagnostic = makeFailureDiagnostic(
      parent, HrnSectionFailureStage::GeometryValidation,
      HrnSectionFailureReason::FixedBoundaryInsufficient, 7, 13);
  HrnSectionConflict conflict;
  conflict.kind = HrnSectionConflictKind::OutsideParent;
  conflict.lhs = parent;
  conflict.has_rhs = true;
  conflict.rhs = HrnSectionTarget::sideRoot(ZeHrnSide::Negative);
  conflict.has_measured_value = true;
  conflict.measured_value = 0.5;
  conflict.has_required_value = true;
  conflict.required_value = 1;
  conflict.has_lhs_witness = true;
  conflict.lhs_witness = {5, 0};
  conflict.has_rhs_witness = true;
  conflict.rhs_witness = {4.5, 0};
  diagnostic.conflicts.push_back(conflict);
  HrnSectionAdjustment adjustment;
  adjustment.kind = HrnSectionAdjustmentKind::IncreaseParentDimension;
  adjustment.target = parent;
  adjustment.parameter = HrnSectionParameterKey::Radius;
  adjustment.has_suggested_value = true;
  adjustment.suggested_value = HrnSectionAdjustmentValue::number(5.5);
  adjustment.certainty = HrnSectionAdjustmentCertainty::SufficientForCurrentPlacement;
  diagnostic.adjustments.push_back(adjustment);

  EXPECT_EQ(diagnostic.stage, HrnSectionFailureStage::GeometryValidation);
  EXPECT_EQ(diagnostic.algorithm_version, 7u);
  EXPECT_EQ(diagnostic.iterations_used, 13u);
  EXPECT_TRUE(diagnostic.conflicts[0].has_measured_value);
  EXPECT_TRUE(diagnostic.conflicts[0].has_required_value);
  EXPECT_TRUE(diagnostic.conflicts[0].has_lhs_witness);
  EXPECT_TRUE(diagnostic.conflicts[0].has_rhs_witness);
  EXPECT_EQ(diagnostic.adjustments[0].certainty,
            HrnSectionAdjustmentCertainty::SufficientForCurrentPlacement);
  EXPECT_EQ(diagnostic.adjustments[0].suggested_value.number_value, 5.5);
}

TEST(HrnSectionDiagnostics, KeepsSearchFailuresDistinct) {
  const auto target = HrnSectionTarget::positionPoint();
  const auto noSolution = makeFailureDiagnostic(target, HrnSectionFailureStage::LayoutSolve,
      HrnSectionFailureReason::NoSolutionFound, 1, 100);
  const auto budget = makeFailureDiagnostic(target, HrnSectionFailureStage::LayoutSolve,
      HrnSectionFailureReason::IterationBudgetExceeded, 1, 100);
  const auto internal = makeFailureDiagnostic(target, HrnSectionFailureStage::Commit,
      HrnSectionFailureReason::InternalError, 1, 0);
  EXPECT_NE(noSolution.reason, budget.reason);
  EXPECT_NE(budget.reason, HrnSectionFailureReason::ConstraintInfeasible);
  EXPECT_TRUE(internal.adjustments.empty());
  HrnSectionAdjustment increase;
  increase.kind = HrnSectionAdjustmentKind::IncreaseIterationBudget;
  increase.parameter = HrnSectionParameterKey::MaxIterations;
  increase.certainty = HrnSectionAdjustmentCertainty::DeterministicCandidate;
  increase.has_suggested_value = true;
  increase.suggested_value = HrnSectionAdjustmentValue::count(200);
  EXPECT_EQ(increase.suggested_value.count_value, 200u);
  HrnSectionAdjustment heuristic;
  heuristic.certainty = HrnSectionAdjustmentCertainty::HeuristicSuggestion;
  EXPECT_EQ(heuristic.certainty, HrnSectionAdjustmentCertainty::HeuristicSuggestion);
}
