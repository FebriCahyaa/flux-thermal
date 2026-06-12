<template>
  <div class="page flex flex-col overflow-y-auto">
    <div class="h-[var(--window-inset-top,0px)] shrink-0" />

    <div class="px-4 pt-4 pb-2 flex items-center gap-3 shrink-0">
      <button @click="$router.back()"
              class="p-2 -ml-2 rounded-full hover:bg-surface-container active:scale-95 transition-transform">
        <ArrowLeftIcon class="w-5 h-5 text-on-background" />
      </button>
      <h1 class="text-xl font-semibold text-on-background">{{ t('settings.log_level') }}</h1>
    </div>

    <div class="mx-4 rounded-2xl bg-surface-container divide-y divide-outline-variant/30 overflow-hidden shrink-0">
      <button v-for="(label, idx) in levels" :key="idx"
              @click="selectLevel(idx)"
              class="w-full flex items-center gap-3 px-4 py-3.5 active:bg-surface-variant/30">
        <span class="flex-1 text-sm text-left">{{ label }}</span>
        <CheckCircleIcon v-if="cfg.prefs.log_level === idx"
                         class="w-5 h-5 text-primary shrink-0" />
      </button>
    </div>

    <div class="shrink-0 h-20" />
  </div>
</template>

<script setup>
import { useI18n } from 'vue-i18n'
import { useRouter } from 'vue-router'
import { useThermalConfigStore } from '@/stores/ThermalConfig'
import ArrowLeftIcon   from '@/components/icons/ArrowLeft.vue'
import CheckCircleIcon from '@/components/icons/CheckCircle.vue'

const { t }  = useI18n()
const router = useRouter()
const cfg    = useThermalConfigStore()

const levels = ['Off', 'Critical', 'Error', 'Warning', 'Info', 'Debug', 'Trace']

async function selectLevel(idx) {
  cfg.prefs.log_level = idx
  await cfg.saveConfig()
  router.back()
}
</script>
