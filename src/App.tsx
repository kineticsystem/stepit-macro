import { useEffect } from 'react';
import { followCamera } from './camera/store';
import { followCommander } from './commander/store';
import { CameraSettings } from './components/CameraSettings';
import { ConnectionBadge } from './components/ConnectionBadge';
import { LightsCard } from './components/LightsCard';
import { LiveView } from './components/LiveView';
import { MotionCard } from './components/MotionCard';
import { SettingsMenu } from './components/SettingsMenu';
import { ShotCard } from './components/ShotCard';
import { TaskStatus } from './components/TaskStatus';
import { followLights } from './freezer/lights';
import { followMotion } from './motion/store';

/**
 * The whole rig on one page, for a tablet or a desktop: what the camera sees
 * and the sliders on the left, the shot, the lights and the camera's settings
 * on the right. On a narrow screen, one below the other.
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
        <div className="main-column">
          <LiveView />
          <MotionCard />
        </div>
        <aside className="side">
          <ShotCard />
          <LightsCard />
          <CameraSettings />
        </aside>
      </main>
    </div>
  );
}
