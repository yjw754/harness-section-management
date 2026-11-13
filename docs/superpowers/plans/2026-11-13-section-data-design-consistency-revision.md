# Section Data Design Consistency Revision Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the project goals and the position-point section data design consistently describe a Segment-driven, per-side section parameter model.

**Architecture:** Section stores shape, size, rotation, and layout parameters only. Each Position Point side derives its Section hierarchy and roots from that side's Segment list, while calculated locations remain parallel data that are fully recomputed after rotation.

**Tech Stack:** Markdown design documents and shell-based consistency checks.

**Spec:** `docs/superpowers/specs/2026-11-13-harness-position-point-section-data-design.md`

## Global Constraints

- Treat each side's Segment list as the authority for Section identity and hierarchy.
- Do not add a separate Section tree, Section parent ID, or parent-relative `x/y` transform.
- Preserve the existing Segment, Location, and Section parallel-list organization.
- Limit current correctness claims to Section management at one Position Point; exclude three-dimensional form and along-path orientation continuity.
- Do not silently claim detection of same-length Section-list permutations, because Section records do not store Segment IDs.

## Review Focus

- A Segment whose parent is absent from one side but present on the other must have independently calculated Root status on each side.
- Rotating any Section must trigger recomputation of all Section locations at the affected Position Point, without relying on stored parent-relative transforms.
- Missing, extra, invalid, or duplicate entries discovered during loading must have deterministic validation and diagnostic behavior.
- Root Section storage must use DB indices consistently with the Segment Section lists.
- Project goals must not retain requirements for an independent Section tree, arbitrary Section reparenting, or persisted local `x/y` transforms.

---

### Task 1: Align the Project Goals with the Segment-Driven Model

**Files:**
- Modify: `线束截面管理项目目标.md`

**Interfaces:**
- Consumes: The approved decisions in the design discussion.
- Produces: A scope and requirements document in which Section is a parameter record associated with a Segment on one Position Point side.

- [ ] **Step 1: Replace independent Section-tree requirements**

State that hierarchy, direct parentage, and Root status are derived from the Segment topology induced by each side's Segment list; remove independent Section-tree creation and Section reparenting requirements.

- [ ] **Step 2: Replace local-transform requirements**

Remove persisted parent-relative `x/y`; retain independent Section rotation and state that rotation causes full location recomputation for the Position Point.

- [ ] **Step 3: Narrow the current project scope**

Make single-Position-Point Section management the current target and move along-path continuity, Loft, sweep, and other three-dimensional form behavior outside the current stage.

- [ ] **Step 4: Update MVP, acceptance criteria, milestones, and risks**

Remove acceptance criteria that depend on an independently editable Section tree and replace them with per-side Segment-derived hierarchy, deterministic recomputation, and list-integrity validation criteria.

### Task 2: Complete the Position-Point Data Design Contracts

**Files:**
- Modify: `docs/superpowers/specs/2026-11-13-harness-position-point-section-data-design.md`

**Interfaces:**
- Consumes: The aligned project goals from Task 1.
- Produces: Explicit Root, recomputation, DB-index ownership, and load-validation contracts.

- [ ] **Step 1: Define side-local Root semantics**

Define a Root Segment on side `S` as a Segment whose parent is absent from side `S`'s Segment list. State explicitly that the same Segment may be a Root on one side and non-Root on the other.

- [ ] **Step 2: Define rotation recomputation semantics**

State that Section does not persist parent-relative position; after rotation, all Segment Section locations at the Position Point are recalculated and the corresponding Location lists are replaced atomically.

- [ ] **Step 3: Use DB indices for Root Sections**

Replace `std::unique_ptr` Root Section fields with DB-index fields and align Root Section ownership/copy language with indexed Segment Section storage.

- [ ] **Step 4: Add validation and recovery rules**

Make the Segment list authoritative; validate Segment existence, side-local uniqueness, and list lengths. Recompute invalid Location data, create defaults for missing Section entries, diagnose extra entries, and reject unsafe automatic repair of invalid or duplicate Segment identity.

- [ ] **Step 5: Document the undetectable permutation limitation**

State that equal-length Section-list reordering cannot be detected without redundant identity metadata and must be prevented by the unified mutation API.

- [ ] **Step 6: Restate the three-dimensional non-goal**

Limit the present coordinate contract to one Position Point and remove any implication that this stage guarantees three-dimensional shape or along-path orientation continuity.

### Task 3: Verify Cross-Document Consistency

**Files:**
- Verify: `线束截面管理项目目标.md`
- Verify: `docs/superpowers/specs/2026-11-13-harness-position-point-section-data-design.md`

**Interfaces:**
- Consumes: Tasks 1 and 2.
- Produces: Evidence that the approved decisions are present and superseded requirements are absent.

- [ ] **Step 1: Search for superseded concepts**

Run `rg -n '独立.*Section.*树|重设父级|相对父.*x|相对父.*二维变换|unique_ptr<HrnSectionData>.*root' 线束截面管理项目目标.md docs/superpowers/specs/2026-11-13-harness-position-point-section-data-design.md`.

Expected: No normative requirement for those concepts remains; historical comparisons, if any, are explicitly identified as not adopted.

- [ ] **Step 2: Search for every approved contract**

Run `rg -n '单侧|Root|重新计算|DB|唯一性|加载|一致性|三维|同序|错位' 线束截面管理项目目标.md docs/superpowers/specs/2026-11-13-harness-position-point-section-data-design.md`.

Expected: Both documents consistently cover side-local Root calculation, full recomputation, DB-index Root storage, list validation/recovery, and the current three-dimensional non-goal.

- [ ] **Step 3: Review both complete documents**

Read both documents from start to finish and confirm that scope, data ownership, MVP, acceptance criteria, and future-work sections do not contradict the approved model.
