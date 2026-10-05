import react from '@vitejs/plugin-react';
import { defineConfig } from 'vite';

// The development server, with hot reload, on port 5176: the rig's container
// shares the host's network, where the editor's takes 5173, the camera's test
// page 5174 and the Freezer's board page 5175. The page reaches the rig on the
// ports of the host that serves it, as in production.
export default defineConfig({
  plugins: [react()],
  server: { host: '0.0.0.0', port: 5176, strictPort: true },
  build: { outDir: 'dist', emptyOutDir: true },
});
