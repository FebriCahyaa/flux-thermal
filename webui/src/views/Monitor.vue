<template>
  <div class="page flex flex-col overflow-y-auto">
    <div class="h-[var(--window-inset-top,0px)] shrink-0" />

    <!-- Header -->
    <div class="px-4 pt-4 pb-2 flex items-center justify-between shrink-0">
      <div>
        <h1 class="text-2xl font-semibold text-on-background">{{ t('monitor.title') }}</h1>
        <p class="text-sm text-on-surface-variant mt-0.5">{{ t('monitor.subtitle') }}</p>
      </div>
      <button @click="store.fetchThermalStatus()"
              class="p-2 rounded-full hover:bg-surface-container active:scale-95 transition-transform">
        <RefreshIcon class="w-5 h-5 text-on-surface-variant" :class="{'animate-spin': store.loading}" />
      </button>
    </div>

    <!-- Max temp summary card -->
    <div class="mx-4 mb-3 rounded-2xl bg-surface-container px-4 py-4 shrink-0">
      <p class="text-xs text-on-surface-variant uppercase tracking-wider">{{ t('monitor.max_temp') }}</p>
      <div class="flex items-end gap-2 mt-1">
        <span class="text-4xl font-bold" :class="store.tempColor(store.maxTempC)">
          {{ store.maxTempC.toFixed(1) }}
        </span>
        <span class="text-xl text-on-surface-variant mb-1">°C</span>
      </div>
      <!-- Temp bar -->
      <div class="mt-3 h-2 rounded-full bg-surface-variant overflow-hidden">
        <div class="h-full rounded-full transition-all duration-700"
             :class="tempBarColor"
             :style="{ width: tempBarWidth }" />
      </div>
      <p v-if="store.lastUpdated" class="text-xs text-on-surface-variant mt-2">
        {{ t('monitor.last_updated') }}: {{ store.lastUpdated }}
      </p>
    </div>

    <!-- Error state -->
    <div v-if="store.error"
         class="mx-4 mb-3 rounded-2xl border border-red-500/30 bg-red-500/10 px-4 py-3 shrink-0">
      <p class="text-sm text-red-400">{{ store.error }}</p>
    </div>

    <!-- Loading skeleton -->
    <div v-else-if="store.loading && store.zones.length === 0"
         class="mx-4 space-y-2 shrink-0">
      <div v-for="i in 6" :key="i"
           class="h-14 rounded-2xl bg-surface-container animate-pulse" />
    </div>

    <!-- No zones -->
    <div v-else-if="!store.loading && store.zones.length === 0"
         class="mx-4 rounded-2xl bg-surface-container px-4 py-6 text-center shrink-0">
      <ThermostatIcon class="w-10 h-10 text-on-surface-variant mx-auto mb-2" />
      <p class="text-sm text-on-surface-variant">{{ t('monitor.no_zones') }}</p>
    </div>

    <!-- Zone list -->
    <div v-else class="mx-4 space-y-2 shrink-0">
      <div v-for="zone in sortedZones" :key="zone.id"
           :class="['rounded-2xl border px-4 py-3 flex items-center gap-3 transition-colors', store.tempBgColor(zone.temp_c)]">
        <!-- Temp indicator dot -->
        <div class="w-2 h-2 rounded-full shrink-0"
             :class="store.tempColor(zone.temp_c).replace('text-', 'bg-')" />

        <!-- Zone info -->
        <div class="flex-1 min-w-0">
          <p class="text-sm font-medium truncate">{{ zone.type || 'zone_' + zone.id }}</p>
          <p class="text-xs text-on-surface-variant">
            {{ t('monitor.zone_policy') }}: {{ zone.policy || '—' }}
          </p>
        </div>

        <!-- Temperature -->
        <div class="text-right shrink-0">
          <span class="text-lg font-semibold" :class="store.tempColor(zone.temp_c)">
            {{ zone.temp_c.toFixed(1) }}
          </span>
          <span class="text-sm text-on-surface-variant">°C</span>
        </div>
      </div>
    </div>

    <div class="shrink-0 h-20" />
  </div>
</template>

<script setup>
import { computed, onMounted, onUnmounted } from 'vue'
import { useI18n } from 'vue-i18n'
import { useMonitorStore } from '@/stores/Monitor'
import RefreshIcon    from '@/components/icons/Refresh.vue'
import ThermostatIcon from '@/components/icons/Thermostat.vue'

const { t }  = useI18n()
const store  = useMonitorStore()

// Sort zones: highest temp first
const sortedZones = computed(() =>
  [...store.zones].sort((a, b) => b.temp_c - a.temp_c)
)

// Progress bar (0–100°C range)
const tempBarWidth = computed(() => {
  const pct = Math.min(100, Math.max(0, store.maxTempC / 100 * 100))
  return pct.toFixed(1) + '%'
})

const tempBarColor = computed(() => {
  const c = store.maxTempC
  if (c >= 70) return 'bg-red-500'
  if (c >= 60) return 'bg-orange-500'
  if (c >= 50) return 'bg-yellow-500'
  if (c >= 40) return 'bg-lime-500'
  return 'bg-green-500'
})

onMounted(() => store.startPolling(3000))
onUnmounted(() => store.stopPolling())
</script>
