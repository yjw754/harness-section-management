# Class-Only Section Model Revision Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove DB and persistence terminology while preserving the existing positive/negative parallel-data section model.

**Architecture:** Express Position Point, per-side data, Segment, Section, Shape, and Layout as ordinary C++ classes. Each side retains three parallel containers for Segment references, calculated Locations, and owned Sections; the Root Section remains a separately owned optional Section object.

**Tech Stack:** Markdown and conceptual C++ class declarations.

**Spec:** `docs/superpowers/specs/2026-11-13-harness-position-point-section-data-design.md`

## Global Constraints

- Preserve separate positive and negative sides.
- Preserve three same-length, same-order containers per side: Segment, Location, and Section.
- Section stores no Segment identity, topology, or position.
- Segment topology remains the authority for hierarchy and side-local Root calculation.
- Do not introduce an aggregate item that combines Segment, Location, and Section.
- Remove DB types, DB indices, serialization, persistence, loading, migration, and file-repair contracts.

## Review Focus

- All prior `idx_*`, `ZwDb*`, DB object, loading, and persistence language must be absent from the two target documents.
- The three parallel containers and their same-length/same-order invariant must remain explicit.
- Segment references are non-owning; Section objects and optional Root Sections have explicit class ownership.
- Copying a Position Point deep-copies Section/Shape/Layout objects while retaining references to the same external Segment objects.
- Runtime validation must still reject null or duplicate Segment references and null/shared Section objects without reintroducing storage-layer terminology.

---

### Task 1: Replace the Concrete Data Model with Ordinary Classes

**Files:**
- Modify: `docs/superpowers/specs/2026-11-13-harness-position-point-section-data-design.md`

**Interfaces:**
- Consumes: Existing side independence, Root semantics, recomputation transaction, and parallel-list invariants.
- Produces: `HrnPositionPoint`, `HrnPositionPointSideData`, `HrnSegment`, and `HrnSectionData` conceptual class declarations.

- [ ] **Step 1: Replace DB fields and names**

Replace `ZwDbHrnPosPnt` and all `idx_*` fields with ordinary classes and `std::vector`/`std::unique_ptr` members while preserving three parallel containers per side.

- [ ] **Step 2: Express Segment topology as a class**

Add a conceptual `HrnSegment` class with non-owning parent/child relationships; Section hierarchy continues to be derived from those relationships and the current side's Segment container.

- [ ] **Step 3: Replace DB ownership contracts**

Describe Section and Root Section ownership with `std::unique_ptr`; describe Segment entries as non-owning references and copying as Section deep-copy plus Segment-reference preservation.

- [ ] **Step 4: Remove storage lifecycle sections**

Delete loading repair, serialization, migration, DB aliases, invalid indices, and file-corruption recovery. Retain only runtime class invariants and atomic mutation/recomputation behavior.

- [ ] **Step 5: Rewrite examples and APIs**

Use `HrnPositionPointSideData` container names and ordinary class accessors throughout; no API or prose may use `idx_*`, `ZwDb*`, DB, index-object identity, or loading terminology.

### Task 2: Align the Project Goals and Verify the Abstraction Boundary

**Files:**
- Modify: `线束截面管理项目目标.md`
- Verify: `docs/superpowers/specs/2026-11-13-harness-position-point-section-data-design.md`

**Interfaces:**
- Consumes: Task 1's class-only model.
- Produces: Project goals that specify runtime object relationships without storage implementation commitments.

- [ ] **Step 1: Remove storage-layer requirements**

Remove DB, index, loading, persistence, serialization, migration, file-reopen, and repair language from current goals, MVP, acceptance criteria, milestones, and risks.

- [ ] **Step 2: Retain runtime consistency goals**

Keep per-side parallel-container equality/order, unique non-null Segment references, independent Section ownership, controlled mutation, and full Position Point recomputation.

- [ ] **Step 3: Run absence checks**

Run `rg -n -i 'DB|idx_|ZwDb|索引|加载|持久化|序列化|迁移|旧文件|重新打开'` against both target documents.

Expected: no matches.

- [ ] **Step 4: Run presence checks**

Verify both documents retain positive/negative side independence, three parallel containers, same-length/same-order invariants, side-local Root calculation, class ownership, and full Position Point recomputation.

- [ ] **Step 5: Review both complete documents**

Confirm that no storage-layer concern has leaked back into the class design and that no previous business invariant was accidentally removed.
