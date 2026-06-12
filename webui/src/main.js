import { createApp } from 'vue'
import { createPinia } from 'pinia'
import { createI18n } from 'vue-i18n'

import App from './App.vue'
import router from './router/index'
import {
  getPreferredLanguage,
  loadLocaleMessages,
  setI18n,
} from './helpers/Locales'

import './assets/main.css'

// ── i18n ──────────────────────────────────────────────────────────────────────

const { locale: preferredLocale } = await getPreferredLanguage()
const localeMessages = await loadLocaleMessages(preferredLocale)

const messages = { [preferredLocale]: localeMessages }
const locale   = preferredLocale

const i18n = createI18n({
  legacy: false,
  locale,
  fallbackLocale: 'en',
  messages,
})

setI18n(i18n)

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
