# The Live View Stays Off After a Test Shot

Pull request [#114](https://github.com/kineticsystem/stepit-macro/pull/114), branch `keep-live-view-off-after-a-test-shot`.

## Goal

`TakeShot` switched the live view on again after the shot, if it was on. StepIt UI shows the picture only while the live view is off, so the picture of a test shot could not be reviewed.

## Decisions

**`TakeShot` leaves the live view off,** as `FocusStack` does: `PauseLiveView resume="false"`. The page shows the picture, and the Live view button brings the live view back.

## Open Items

- A picture-in-picture view in StepIt UI, the picture and the live view together, one large and one small, swapped by a tap. `TakeShot` could then switch the live view on again.
