import { useEffect } from 'react';
import { followCamera } from './camera/store';
import { followCommander } from './commander/store';
import { ConnectionBadge } from './components/ConnectionBadge';
import { LiveView } from './components/LiveView';
import { RailMark } from './components/RailMark';
import { StageMark } from './components/StageMark';
import { SettingsMenu } from './components/SettingsMenu';
import { StackBar } from './components/StackBar';
import { Slider } from './components/Slider';
import { TaskStatus } from './components/TaskStatus';
import { Toolbar } from './components/Toolbar';
import { followLights } from './freezer/lights';
import { followMotion, SLIDERS, useMotion } from './motion/store';
import { useSettings } from './settings';
import { followStack } from './stack/store';

/**
 * The whole rig on one page, for a tablet held in both hands: a slider at each
 * edge, under each thumb, the rotary stage on the left and the rail on the
 * right; the live view in the middle, with the commands above it, and the
 * focus stack under them, which the Stack button shows.
 */
export function App() {
  const showStack = useSettings((s) => s.showStack);
  useEffect(() => {
    const stops = [followCommander(), followCamera(), followLights(), followMotion(), followStack()];
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
          {showStack && <StackBar />}
          <LiveView />
        </div>
        <SideSlider index={1} />
      </main>
    </div>
  );
}

/**
 * The slider of a joint, at one edge of the page. While the stack bar is
 * shown, each carries the ends of the stack at its own ends: where the rail
 * goes between, and the angles the stage turns between.
 */
function SideSlider({ index }: { index: number }) {
  const { enabled, axes, setAxis } = useMotion();
  const showStack = useSettings((s) => s.showStack);
  const slider = SLIDERS[index];
  const marks = showStack && slider.joint === 'joint2';
  const limits = showStack && slider.joint === 'joint1';
  return (
    <aside className="side-slider">
      {marks && <RailMark end="near" />}
      {limits && <StageMark end="to" />}
      <Slider
        label={slider.label}
        value={axes[slider.axis]}
        disabled={!enabled}
        onChange={(value) => setAxis(slider.axis, value)}
      />
      {marks && <RailMark end="far" />}
      {limits && <StageMark end="from" />}
    </aside>
  );
}
