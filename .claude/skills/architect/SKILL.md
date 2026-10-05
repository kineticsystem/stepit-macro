---
name: architect
description: Review the architecture of a StepIt module (modules/<module>) or of the rig's own code (src/plugins, src/stepit-macro, ui), and write or update its docs/ARCHITECTURE.md with a Review section - layers, SOLID, coupling, duplication, verified implementation issues and recommendations. Use when asked to document, review or audit how a module is built, or to refresh an existing ARCHITECTURE.md after changes.
---

# Architect

You review how one part of StepIt Macro is built and write it down in its
`docs/ARCHITECTURE.md`: first how it works, then a **Review** that judges it.
The document is for a developer who has read the README and used the module
once, and wants to know where to start a change and what is fragile.

The argument names the target, e.g. `ui` (StepIt UI), `modules/stepit-camera`,
`src/plugins`. Without one, ask which module.

## 1. Read before writing

- **The house style**: read every existing `modules/*/docs/ARCHITECTURE.md`
  (and `modules/stepit-camera/docs/WEB_PAGE.md` for a web page). Match their
  section order, tone and diagrams; the newest one by `git log -1 --format=%ci
  -- docs/ARCHITECTURE.md` wins when they differ.
- **The target, whole**: every source file, the tests, the build and CI
  files, the README, any CLAUDE.md. Do not review from excerpts: a review
  that has not read a file cannot say it is clean.
- **Its neighbours**: what it talks to in the rig (`rig.yaml`, the
  objectives, the other modules' interfaces), and any code it shares with or
  copied from another module (`diff` the files: copies that diverge are a
  finding).
- An existing `docs/ARCHITECTURE.md` of the target: update it rather than
  start again, keep what is still true, and replace its Review.

## 2. The document

Follow the existing documents. The usual sections, dropping those that do not
apply and adding ones the module needs (e.g. *The Protocol*, *Safety Nets*):

1. `# <Module> Architecture`, then `## Table of Contents <!-- omit in toc -->`.
2. **Introduction**: what the document is for, what it assumes, and the
   **one idea** the module follows, in bold.
3. **The Big Picture**: a diagram of the module and what it talks to, and a
   table of its interface (topics, services, ports, routes).
4. **The Layers** or **The Packages**: a table of folder or package,
   responsibility, what it may know; the dependency direction, as a diagram
   of the imports *as they are*, with any edge that breaks the rule shown.
5. The parts, one section each, in the order a reader needs them; a sequence
   diagram for the longest flow (from a click, or a goal, to the result).
6. **Tests**: a table of test and what it covers, and what is *not* tested.
7. **How to Extend…**: a table of "to…" / "change…".
8. **Design Decisions and Trade-offs**: bold lead sentence, then why.
9. **Review**, last (see below).

Style:

- Plain English, short sentences, "we" for the developers; British spelling
  as in the existing documents (centre, colour).
- Link files relatively (`[`store.ts`](../src/camera/store.ts)`); name
  functions and constants in backticks so they can be searched.
- Mermaid diagrams start with the same `config` block as the newest existing
  `ARCHITECTURE.md`, copied whole, and end with
  `classDef default fill:#3b6fb6,stroke:#2c5590,color:#ffffff` for flowcharts.
- Keep diagrams narrow enough to read on a page: lay flowcharts out
  top to bottom (`flowchart TB`), never a long left-to-right chain; at most
  about five boxes side by side, and four participants in a sequence diagram.
  Boxes hold a name, edge labels a few words: the details go in the text
  under the diagram, not in it. Leave out what the text can say in one line.
- Check every diagram renders before finishing:
  `docker run --rm -u $(id -u):$(id -g) -v <dir>:/data minlag/mermaid-cli -i /data/ARCHITECTURE.md -o /data/out.md`
  on a copy in the scratchpad, then render them with `-e png -s 2` and look
  at each image: a diagram that parses can still be too wide to read.
- Link the document from the module's README if it is not already.

## 3. The Review

Open it with the date and the commit reviewed. Answer each question with a
verdict and the evidence, file and function, not a general impression:

- **Is it well structured?** What is solid, and the one weakest part.
- **Are the layers respected?** Each import or call that skips a layer, or
  knowledge that lives in the wrong one (protocol names in state, rig names in
  views, policy in components).
- **SOLID**: a table, one row per principle, with a verdict and the place
  that proves it. Say "barely applies" when it does (e.g. Liskov without
  inheritance) rather than inventing a finding.
- **Coupling**: which parts cannot change apart, and which rules are spread
  over several places.
- **Duplication**: a table of what, where, and how much it matters. Include
  copies across modules and values kept "in step by hand" with `rig.yaml`.
- **Implementation issues**: ordered by severity, each with where it is and
  what goes wrong. Say whether any is a safety issue for the rig.
- **Recommendations**: ordered, each small enough for one pull request, each
  tied to the findings it fixes, with the trade-off when there is one.

Rules for findings:

- **Verify what can be verified.** Reproduce a suspected bug with a
  throwaway test in the scratchpad, run inside the rig's image, never on the
  host, e.g. for a web page:
  `docker run --rm -u $(id -u):$(id -g) -v $PWD:/ui -v <scratch>:/ui/tests/probe -w /ui stepit-macro:latest bash -lc 'node_modules/.bin/vitest run tests/probe'`;
  for ROS packages, `./docker/dock.sh shell` and the workspace's `test.sh`.
  Mark verified findings *verified*; the rest come from reading.
- No finding without a place in the code. No praise without one either.
- Respect the rig's deliberate choices (CLAUDE.md, the module's own Design
  Decisions, `docs/*.md` with measurements): judge them, but do not report a
  documented trade-off as a bug.
- When a finding is fixed or a recommendation done, remove it entirely and
  renumber the rest: no "Fixed" or "Done" marks. If the fix changed how the
  module works, describe the new behaviour in the sections above instead.
- Do not fix anything while reviewing. The review is the deliverable; offer
  the fixes afterwards.

## 4. Finish

- Leave the changes uncommitted unless asked. A module is a submodule with
  its own repository: committing there, then `git add modules/<module>` here,
  is the user's call.
- Report to the user: where the document is, the verdict in a few lines, the
  most serious findings, and what was verified versus read.
