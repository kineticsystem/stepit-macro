import { useEffect } from 'react';
import { followCamera } from './camera/store';
import { followCommander } from './commander/store';
import { ConnectionBadge } from './components/ConnectionBadge';
import { LiveView } from './components/LiveView';
import { RailMark } from './components/RailMark';
import { RailIcon, RotateIcon } from './components/icons';
import { SettingsMenu } from './components/SettingsMenu';
import { StackBar } from './components/StackBar';
import { Slider } from './components/Slider';
import { TaskStatus } from './components/TaskStatus';
import { Toolbar } from './components/Toolbar';
import { followLights } from './freezer/lights';
import { followMotion, SLIDERS, useMotion } from './motion/store';
import { followPower } from './power/store';
import { followPictures } from './shot/store';
import { followStack } from './stack/store';

/**
 * The whole rig on one page, for a tablet held in both hands: a slider at each
 * edge, under each thumb, the rotary stage on the left and the rail on the
 * right; the live view in the middle, with the commands above it, and the
 * focus stack under it.
 */
export function App() {
  useEffect(() => {
    const stops = [
      followCommander(), followCamera(), followLights(), followMotion(), followStack(), followPictures(), followPower(),
    ];
    return () => stops.forEach((stop) => stop());
  }, []);

  return (
    <div className="app">
      <header className="topbar">
        <span className="brand">StepIt Macro</span>
        <TaskStatus />
        <span className="row-spacer" />
        <ConnectionBadge />
        <SettingsMenu />
      </header>
      <main className="page">
        <SideSlider index={0} />
        <div className="centre">
          <Toolbar />
          <LiveView />
          <StackBar />
        </div>
        <SideSlider index={1} />
      </main>
    </div>
  );
}

/**
 * The slider of a joint, at one edge of the page. The rail's carries the
 * marks of the stack at its ends; the stage's keeps the same room empty, so
 * that both sliders have the same height.
 */
function SideSlider({ index }: { index: number }) {
  const { enabled, axes, setAxis } = useMotion();
  const slider = SLIDERS[index];
  const marks = slider.joint === 'joint2';
  return (
    <aside className="side-slider">
      {marks ? <RailMark end="near" /> : <MarkSpace end="top" />}
      <Slider
        label={slider.label}
        icon={slider.joint === 'joint1' ? <RotateIcon /> : <RailIcon />}
        value={axes[slider.axis]}
        disabled={!enabled}
        onChange={(value) => setAxis(slider.axis, value)}
      />
      {marks ? <RailMark end="far" /> : <MarkSpace end="bottom" />}
    </aside>
  );
}

/** The room of a Mark button, empty: laid out as one, never seen nor reached. */
function MarkSpace({ end }: { end: 'top' | 'bottom' }) {
  return (
    <div className={`rail-mark ${end} mark-space`} aria-hidden="true">
      <button tabIndex={-1}>Mark</button>
    </div>
  );
}
