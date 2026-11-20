# Coupled circular parent repair

The Stage 2 debugger must solve the original zero-rotation asymmetric case with three same-position pairs. Keep the existing rule-based layout when it already satisfies the coupled constraints. Keep the input schema, hierarchy, radii of fixed sections, rotation settings, and root anchoring rules unchanged.

## Approaches

- Chosen: bounded local repair after the first placement hits an overlap. An ancestor with a mapped descendant is not itself mapped; it may change center while its children retain their centers. Recompute its automatic radius before accepting that move. After a coupled `NoSolutionFound` (including fixed-boundary failure), try the other side's independent centers once. Any successful attempt must pass the existing final geometry checks.
- A complete joint search over all centers would cover more cases but changes the solver and its performance substantially. Leave it out of this change. A manual 180° Root rotation is a review workaround, not a fix.

An overlap in a candidate does not establish impossibility. Keep hard-conflict reporting for provable contradictions, such as siblings forced to the same center, and report exhausted placement candidates as `NoSolutionFound`. Existing successful layouts stay on the original path. The original and side-swapped cases must solve, and a genuine hard conflict must retain its witnesses.
