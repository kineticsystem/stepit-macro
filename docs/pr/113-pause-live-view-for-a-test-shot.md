# The Live View Off for the Shots

Pull request [#113](https://github.com/kineticsystem/stepit-macro/pull/113), branch `pause-live-view-for-a-test-shot`.

## Goal

A camera in live view ignores a shot: a test shot taken with the live view on always failed. StepIt UI switched the live view off before a shot, but only when the page thought it was on, and it did not wait for the camera to leave it.

## Decisions

**A behavior, `PauseLiveView`.** A decorator: it reads `streaming` on `/camera/status`; when the live view is on, it calls `/camera/stop_streaming` and waits for its answer, which StepIt Camera [#18](https://github.com/kineticsystem/stepit-camera/pull/18) gives once the camera has left the live view; then it runs its child. Its port `resume` decides whether it switches the live view on again after the child, however the child ends.

**`TakeShot` resumes it.** A test shot is taken while framing: the live view comes back after it, if it was on.

**`FocusStack` does not.** A stack takes long, and the camera would drain its battery in live view after it: the live view stays off.

**The objectives do it, not the page.** StepIt UI no longer stops the live view before a shot or a stack: any client that runs the objectives gets the same.

**`modules/stepit-camera` moves to #18.** `PauseLiveView` needs the camera to answer `stop_streaming` once the live view is off.

## Open Items

- Merge StepIt Camera #18 first, then this one.
