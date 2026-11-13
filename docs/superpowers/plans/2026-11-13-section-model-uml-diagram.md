# Section Model UML Diagram Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an inline Mermaid UML class diagram to the position-point section data design.

**Architecture:** The diagram is a visual projection of the existing Class-only model. It shows ownership, non-owning topology references, inheritance, cardinality, and the unchanged three-parallel-container invariant without adding new classes or relationships.

**Tech Stack:** Markdown and Mermaid `classDiagram`.

**Spec:** `docs/superpowers/specs/2026-11-13-harness-position-point-section-data-design.md`

## Global Constraints

- Add the diagram as section `5.3 UML 类图`.
- Do not change the existing data model or prose contracts.
- Keep Segment, Location, and Section as three parallel containers.
- Show Segment links as non-owning associations and Section links as composition/ownership.
- Show Shape and Layout inheritance exactly as declared in the document.

## Review Focus

- `HrnPositionPoint` owns exactly two `HrnPositionPointSideData` objects: positive and negative.
- `HrnPositionPointSideData` contains three parallel containers and an optional Root Section.
- Segment parent/child topology is self-associated and non-owning.
- Section owns one Shape and one Layout; concrete Shape/Layout classes inherit from their abstract bases.
- The diagram must not contain DB, index, persistence, loading, or aggregate-item concepts.

---

### Task 1: Add and Verify the Mermaid Class Diagram

**Files:**
- Modify: `docs/superpowers/specs/2026-11-13-harness-position-point-section-data-design.md`

**Interfaces:**
- Consumes: Existing `HrnPositionPoint`, `HrnPositionPointSideData`, `HrnSegment`, `HrnSectionData`, Shape, and Layout declarations.
- Produces: An inline UML overview consistent with those declarations.

- [ ] **Step 1: Add section `5.3 UML 类图`**

Insert the section after the multi-Root discussion and before the detailed Section/Shape/Layout chapter.

- [ ] **Step 2: Add Mermaid classes and relationships**

Include the four core model classes, abstract Shape/Layout bases, five concrete strategies, composition/association arrows, cardinalities, and relevant container fields.

- [ ] **Step 3: Add the parallel-container invariant note**

State that `segments.size == locations.size == sections.size`, and that Root Section exists only when the side has multiple Root Segments.

- [ ] **Step 4: Verify diagram syntax and semantic coverage**

Check that the Mermaid fence is balanced, every documented class is present, all expected relationships appear, and forbidden storage-layer terms remain absent from the design document.

- [ ] **Step 5: Review the complete chapter**

Confirm that the diagram does not imply ownership of Segment objects, does not combine the parallel containers, and matches the surrounding prose.
