import { createRouter, createWebHistory } from 'vue-router'
import { WXEventHandler } from 'webuix'

import Home     from '@/views/Home.vue'
import Monitor  from '@/views/Monitor.vue'
import Settings from '@/views/Settings.vue'

// Sub-pages
import LogLevelSelection  from '@/views/LogLevelSelection.vue'
import LanguageSelection  from '@/views/LanguageSelection.vue'
import ThresholdSettings  from '@/views/ThresholdSettings.vue'

window.wx = new WXEventHandler()

const routes = [
  { path: '/',         name: 'Home',     component: Home     },
  { path: '/monitor',  name: 'Monitor',  component: Monitor  },
  { path: '/settings', name: 'Settings', component: Settings },

  // Settings sub-pages
  { path: '/settings/log_level',   name: 'LogLevelSelection', component: LogLevelSelection },
  { path: '/settings/language',    name: 'LanguageSelection', component: LanguageSelection },
  { path: '/settings/thresholds',  name: 'ThresholdSettings', component: ThresholdSettings },

  // catch-all
  { path: '/:pathMatch(.*)*', redirect: '/' },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

// Handle hardware back button (WebUI X API)
wx.on(window, 'back', () => {
  const current = router.currentRoute.value.path || '/'
  if (current === '/') {
    webui.exit()
    return
  }
  const segments = current.split('/').filter(Boolean)
  const parentPath = segments.length > 1 ? '/' + segments.slice(0, -1).join('/') : '/'
  router.push(parentPath)
})

export default router
