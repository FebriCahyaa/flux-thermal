import { createApp } from 'vue'
import { createPinia } from 'pinia'
import { createI18n } from 'vue-i18n'

import App from './App.vue'
import router from './router/index'
import { Locales } from './helpers/Locales'

import './assets/main.css'

// ── i18n ──────────────────────────────────────────────────────────────────────

const messages = await Locales.loadAll()
const locale   = Locales.getPreferredLocale(Object.keys(messages))

const i18n = createI18n({
  legacy: false,
  locale,
  fallbackLocale: 'en',
  messages,
})

// ── Eruda (dev mode only) ─────────────────────────────────────────────────────

if (import.meta.env.VITE_ENABLE_ERUDA === 'true') {
  const { default: eruda } = await import('eruda')
  eruda.init()
}

// ── App ───────────────────────────────────────────────────────────────────────

const app = createApp(App)
app.use(createPinia())
app.use(router)
app.use(i18n)
app.mount('#app')
