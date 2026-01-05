import { mount } from 'svelte'
import './app.css'
import App from './App.svelte'

// Initialize WebBridge runtime (auto-initializes on import)
import './webbridge'

const app = mount(App, {
  target: document.getElementById('app')!,
})

export default app
