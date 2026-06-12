<template>
  <!-- Desktop sidebar -->
  <nav class="hidden md:flex flex-col items-center fixed left-0 inset-y-0 w-20 bg-surface-container
              border-r border-outline-variant/30 gap-1 pt-[var(--window-inset-top,0px)] z-30">
    <RouterLink v-for="item in navItems" :key="item.to" :to="item.to"
      :class="['flex flex-col items-center justify-center gap-1 py-3 w-full cursor-pointer transition-colors',
               isActive(item.to) ? 'text-primary' : 'text-on-surface-variant']">
      <div :class="['w-14 h-8 flex items-center justify-center rounded-full transition-colors',
                    isActive(item.to) ? 'bg-secondary-container' : 'hover:bg-surface-variant/50']">
        <component :is="item.icon" class="w-5 h-5" />
      </div>
      <span class="text-[10px] font-medium">{{ item.label }}</span>
    </RouterLink>
  </nav>

  <!-- Mobile bottom bar -->
  <nav class="md:hidden fixed bottom-0 inset-x-0 z-30 bg-surface-container
              border-t border-outline-variant/30 flex items-center
              pb-[env(safe-area-inset-bottom)]">
    <RouterLink v-for="item in navItems" :key="item.to" :to="item.to"
      :class="['flex-1 flex flex-col items-center justify-center gap-1 py-2 cursor-pointer transition-colors',
               isActive(item.to) ? 'text-primary' : 'text-on-surface-variant']">
      <div :class="['w-14 h-8 flex items-center justify-center rounded-full transition-colors',
                    isActive(item.to) ? 'bg-secondary-container' : '']">
        <component :is="item.icon" class="w-5 h-5" />
      </div>
      <span class="text-[10px] font-medium">{{ item.label }}</span>
    </RouterLink>
  </nav>
</template>

<script setup>
import { computed } from 'vue'
import { useRoute } from 'vue-router'
import { useI18n  } from 'vue-i18n'
import { RouterLink } from 'vue-router'

import HomeIcon     from '@/components/icons/Home.vue'
import MonitorIcon  from '@/components/icons/Monitor.vue'
import SettingsIcon from '@/components/icons/Settings.vue'

const { t }  = useI18n()
const route  = useRoute()

const navItems = computed(() => [
  { to: '/',         label: t('nav.home'),     icon: HomeIcon     },
  { to: '/monitor',  label: t('nav.monitor'),  icon: MonitorIcon  },
  { to: '/settings', label: t('nav.settings'), icon: SettingsIcon },
])

function isActive(path) {
  if (path === '/') return route.path === '/'
  return route.path.startsWith(path)
}
</script>
