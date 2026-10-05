import { useEffect } from 'react';
import { followCamera } from './camera/store';
import { followCommander } from './commander/store';
import { ConnectionBadge } from './components/ConnectionBadge';
import { LiveView } from './components/LiveView';
import { SettingsMenu } from './components/SettingsMenu';
import { Slider } from './components/Slider';
import { TaskStatus } from './components/TaskStatus';
import { Toolbar } from './components/Toolbar';
import { followLights } from './freezer/lights';
import { followMotion, SLIDERS, useMotion } from './motion/store';

/**
 * The whole rig on one page, for a tablet held in both hands: a slider at each
 * edge, under each thumb, the rotary stage on the left and the rail on the
 * right; the live view in the middle, with the commands above it.
 */
export function App() {
  useEffect(() => {
    const stops = [followCommander(), followCamera(), followLights(), followMotion()];
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
        </div>
        <SideSlider index={1} />
      </main>
    </div>
  );
}

/** The slider of a joint, at one edge of the page. */
function SideSlider({ index }: { index: number }) {
  const { enabled, axes, setAxis } = useMotion();
  const slider = SLIDERS[index];
  return (
    <aside className="side-slider">
      <Slider
        label={slider.label}
        maxTurnsPerSecond={slider.maxTurnsPerSecond}
        value={axes[slider.axis]}
        disabled={!enabled}
        onChange={(value) => setAxis(slider.axis, value)}
      />
    </aside>
  );
}
