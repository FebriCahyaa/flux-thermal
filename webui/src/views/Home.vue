<template>
  <div class="page flex flex-col overflow-y-auto">
    <!-- Status bar spacer -->
    <div class="h-[var(--window-inset-top,0px)] shrink-0" />

    <!-- Header -->
    <div class="px-4 pt-4 pb-2 shrink-0">
      <h1 class="text-2xl font-semibold text-on-background">Flux Thermal</h1>
      <p class="text-sm text-on-surface-variant mt-0.5">{{ moduleVersion }}</p>
    </div>

    <!-- Dependency Error Banner -->
    <div v-if="dependencyError"
         class="mx-4 mb-3 rounded-2xl border border-red-500/40 bg-red-500/10 p-4 flex gap-3 items-start shrink-0">
      <ErrorIcon class="w-6 h-6 text-red-500 shrink-0 mt-0.5" />
      <div>
        <p class="font-semibold text-red-500">{{ t('home.flux_dependency_error') }}</p>
        <p class="text-sm text-on-surface-variant mt-1">{{ t('home.flux_dependency_desc') }}</p>
      </div>
    </div>

    <!-- Daemon + Profile Card -->
    <div class="mx-4 mb-3 rounded-2xl bg-surface-container overflow-hidden shrink-0">
      <!-- Daemon status row -->
      <div class="flex items-center gap-3 px-4 py-3 border-b border-outline-variant/30">
        <span :class="['w-2.5 h-2.5 rounded-full shrink-0', daemonDot]" />
        <span class="text-sm font-medium flex-1">{{ daemonLabel }}</span>
        <span v-if="daemonPidRaw" class="text-xs text-on-surface-variant font-mono">
          PID {{ daemonPidRaw }}
        </span>
      </div>

      <!-- Profile display -->
      <div class="flex items-center gap-4 px-4 py-4">
        <!-- Animated thermometer icon -->
        <div :class="['w-14 h-14 rounded-full flex items-center justify-center shrink-0', profileBg]">
          <ThermostatIcon class="w-7 h-7" :class="profileIconColor" />
        </div>
        <div class="flex-1 min-w-0">
          <p class="text-xs text-on-surface-variant uppercase tracking-wider">
            {{ t('home.profile_label') }}
          </p>
          <p class="text-xl font-semibold mt-0.5" :class="profileColor">
            {{ profileLabel }}
          </p>
          <p class="text-xs text-on-surface-variant mt-0.5">{{ profileDesc }}</p>
        </div>
      </div>
    </div>

    <!-- Device info cards -->
    <div class="mx-4 grid grid-cols-2 gap-3 mb-3 shrink-0">
      <InfoCard :label="t('home.chipset')"         :value="chipsetName"   icon="chipset"  />
      <InfoCard :label="t('home.android_sdk')"     :value="'API ' + androidSDK" icon="android" />
      <InfoCard :label="t('home.kernel')"          :value="kernelVersion" icon="code"   class="col-span-2" />
    </div>

    <!-- Bottom spacer for nav bar -->
    <div class="shrink-0 h-20" />
  </div>
</template>

<script setup>
import { computed, onMounted, onUnmounted } from 'vue'
import { useI18n } from 'vue-i18n'
import { useHomeStore } from '@/stores/Home'
import ThermostatIcon from '@/components/icons/Thermostat.vue'
import ErrorIcon      from '@/components/icons/Error.vue'
import InfoCard       from '@/components/ui/InfoCard.vue'

const { t }  = useI18n()
const store  = useHomeStore()

const {
  daemonPidRaw, moduleVersion, currentProfileRaw,
  kernelVersion, chipsetName, androidSDK,
  daemonStatusRaw, daemonError, dependencyError,
} = store

// ── Daemon UI ─────────────────────────────────────────────────────────────────

const daemonDot = computed(() => ({
  running: 'bg-green-500 shadow-[0_0_6px_2px_rgba(74,222,128,0.4)]',
  stopped: 'bg-red-500',
  error:   'bg-orange-500',
  loading: 'bg-surface-variant animate-pulse',
}[daemonStatusRaw] || 'bg-surface-variant'))

const daemonLabel = computed(() => ({
  running: t('home.daemon_running'),
  stopped: t('home.daemon_stopped'),
  error:   `${t('home.daemon_error')}: ${daemonError}`,
  loading: t('home.daemon_loading'),
}[daemonStatusRaw] || t('home.daemon_loading')))

// ── Profile UI ────────────────────────────────────────────────────────────────

const profileLabel = computed(() => ({
  normal:       t('home.profile_normal'),
  cool:         t('home.profile_cool'),
  warm:         t('home.profile_warm'),
  hot:          t('home.profile_hot'),
  critical:     t('home.profile_critical'),
  initializing: t('home.profile_initializing'),
  unknown:      t('home.profile_unknown'),
}[currentProfileRaw] || t('home.profile_initializing')))

const profileDesc = computed(() => ({
  normal:       'All limits lifted',
  cool:         'Light thermal monitoring active',
  warm:         'Moderate frequency limits applied',
  hot:          'Aggressive throttling in effect',
  critical:     '🚨 Emergency thermal limits — minimum clocks',
  initializing: 'Daemon is starting up…',
  unknown:      'Status unknown',
}[currentProfileRaw] || ''))

const profileColor = computed(() => ({
  normal:       'text-green-500',
  cool:         'text-sky-400',
  warm:         'text-yellow-400',
  hot:          'text-orange-500',
  critical:     'text-red-500',
  initializing: 'text-on-surface-variant',
  unknown:      'text-on-surface-variant',
}[currentProfileRaw] || 'text-on-surface-variant'))

const profileIconColor = computed(() => profileColor.value)

const profileBg = computed(() => ({
  normal:       'bg-green-500/10',
  cool:         'bg-sky-400/10',
  warm:         'bg-yellow-400/10',
  hot:          'bg-orange-500/10',
  critical:     'bg-red-500/10',
  initializing: 'bg-surface-variant',
  unknown:      'bg-surface-variant',
}[currentProfileRaw] || 'bg-surface-variant'))

// ── Lifecycle ─────────────────────────────────────────────────────────────────

onMounted(() => store.initializeData())
onUnmounted(() => {
  store.stopProfileMonitoring()
  store.stopDaemonMonitoring()
})
</script>
