# Estimating the Progress of an Objective

## Table of Contents <!-- omit in toc -->

- [Introduction](#introduction)
- [What Exists](#what-exists)
- [Version 1: the Structure Alone](#version-1-the-structure-alone)
- [Version 2: Time Instead of Fractions](#version-2-time-instead-of-fractions)
  - [Durations from History](#durations-from-history)
- [What No Algorithm Can Know](#what-no-algorithm-can-know)
- [Where to Compute It](#where-to-compute-it)
- [Plan](#plan)

## Introduction

How far along is a running objective, as a whole? A tree has nodes that report their own progress, nodes that do not, and
control nodes that run their children in sequence, in parallel, in loops or as alternatives. This document describes an
algorithm that combines all of them into one percentage, and, with durations, into a time left. Nothing of it is
implemented yet: see [What Exists](#what-exists).

The algorithm is a recursive estimate over the tree, where each control node combines the progress of its children by a
rule of its own. The recursion is the easy part; the hard part is the **weights**, because the progress of two children
is in different units: 3 of 11 iterations and 2.1 of 5 seconds cannot simply be averaged.

## What Exists

A node implementing `stepit_server::ProgressReporter` (StepIt Commander, `stepit_server/progress.hpp`) tells how far it
is while it runs, as `done` out of `total` in a unit of its own. The commander adds it to the feedback of the goal, next
to the status of every node, by `_uid`:

```json
{"nodes": {"3": "RUNNING"}, "progress": {"3": {"done": 2, "total": 11}}}
```

and the StepIt Editor shows it on the row of each running node. Today only `Steps` reports: the iterations done, out of
all of them. Others could, e.g. `FollowJointTrajectory`, exactly, from the time of its trajectory, and
`CommandJointPositions`, by the distance covered.

What is missing is the progress of the objective as a whole, in the header of the execution view: the subject of this
document.

## Version 1: the Structure Alone

Compute `p(node)`, from 0 to 1, bottom-up, from the statuses and progress the editor already receives:

| Node | `p(node)` |
|---|---|
| Ended: SUCCESS, FAILURE, HALTED or SKIPPED | 1 |
| Not reached yet | 0 |
| Running leaf that reports | `done / total` |
| Running leaf that does not report | 0, or ½ if a jump at its end is worse than a stall |
| `Sequence` of N children | `(children ended + p(running child)) / N` |
| `Steps`, `Repeat` with a known count | `(iterations done + p(child)) / count` |
| `Parallel`, every child must succeed | `min p(children)`: it ends with the slowest |
| `Parallel` with a threshold of M | the M-th largest `p(child)` |
| `Fallback` | `p(running child)`, assuming it succeeds: if it fails, the progress falls back as the next child starts |
| `SubTree`, plain decorators | `p(child)` |
| Conditions, and the children a reactive node ticks again | weight 0: instant |
| `RetryUntilSuccessful`, `KeepRunningUntilFailure`, any loop of unknown length | no total: the result is **indeterminate** above them |

For [`Stack`](Stack.md) this is already close to exact: nearly all of its time is spent in its two nested `Steps`, so it
gives

```text
p = (row + (shot + p(move)) / 11) / 11
```

Its weakness is that a `Sequence` counts its children as equal shares. In `Stack`, `EnsureControllers`,
`GetJointPositions`, the outer `Steps` and the move back home would count for 25% each, while `Steps` takes 99% of the
time.

## Version 2: Time Instead of Fractions

The fix is to reason in **seconds left**, `R`, with the **expected duration** of a node, `D`:

| Node | Left, `R` | Expected duration, `D` |
|---|---|---|
| `Sequence` | `R(running child) + Σ D(next children)` | `Σ D(children)` |
| `Parallel`, every child | `max R(children)` | `max D(children)` |
| `Steps`, count n, at iteration k | `R(child) + (n − k − 1) · D(child)` | `n · D(child)` |
| `Fallback` | `R(running child)` | `D(first child)`, optimistic |
| Leaf reporting in seconds, as `FollowJointTrajectory` could | `total − done` | `total` |
| Leaf reporting a fraction `f` after `t` seconds | `t · (1 − f) / f`, extrapolated from its rate | from history |
| Leaf that does not report | `max(0, D − elapsed)` | **from history** |

The progress of the objective is then

```text
p = elapsed / (elapsed + R(root))
```

and `R(root)` is itself a time left. The elapsed time is exact: the editor has the clock.

### Durations from History

The leaves that report nothing take their duration from history: record how long each node took, by objective and
`_uid`, which stays the same as long as the XML does. After one run of `Stack`, every `CommandJointPositions` and
`SwitchController` has a measured duration, and the estimate becomes good. Without history, fall back to the equal
shares of [version 1](#version-1-the-structure-alone).

## What No Algorithm Can Know

- **Which branch a condition or a `Fallback` takes.** The progress can go back when a child fails. Show the largest
  value reached, unless a failure really starts the work again.
- **Loops of unknown length**, e.g. retrying until a success or waiting for an event. Show the progress as
  indeterminate, with the number of iterations instead.
- **Preemption**, which ends the objective early. That is the end of the run, not a problem of progress.

## Where to Compute It

In the StepIt Editor: it already has the executed tree, the status and progress of every node, and the clock. Version 1
is one recursive function over `ExecutedTree`, in the spirit of `failureCauses` in `src/client/execution.ts`, which
fills the header of the execution view. Version 2 adds a small store of durations per objective, in the browser at
first. Moving it into the commander is only worth it once a client other than the editor needs the time left.

## Plan

1. Version 1 in the editor, the indeterminate case included.
2. `FollowJointTrajectory` and `CommandJointPositions` report their progress, in seconds where they can.
3. Version 2, with the durations from history.
