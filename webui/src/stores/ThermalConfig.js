import { defineStore } from 'pinia'
import { ref, reactive } from 'vue'
import { exec } from 'kernelsu'
import * as KernelSU from '@/helpers/KernelSU'

const configPath = '/data/adb/.config/flux_thermal'
const configFile = `${configPath}/config.json`

export const useThermalConfigStore = defineStore('thermalConfig', () => {
  const loaded  = ref(false)
  const saving  = ref(false)
  const saveMsg = ref('')

  const prefs = reactive({
    enabled:              true,
    aggressive_mode:      false,
    protect_battery:      true,
    cool_down_on_charge:  true,
    log_level:            4,
    threshold_cool_mc:    0,
    threshold_warm_mc:    0,
    threshold_hot_mc:     0,
    threshold_critical_mc:0,
  })

  const zoneFilter = reactive({
    include_types: ['cpu', 'gpu', 'soc', 'skin', 'thermal'],
  })

  // ── Load ──────────────────────────────────────────────────────────────────

  async function loadConfig() {
    try {
      const raw  = await KernelSU.readFile(configFile)
      const data = JSON.parse(raw)

      if (data.preferences) {
        Object.assign(prefs, data.preferences)
      }
      if (data.zone_filter?.include_types) {
        zoneFilter.include_types = [...data.zone_filter.include_types]
      }
      loaded.value = true
    } catch {
      loaded.value = true // use defaults
    }
  }

  // ── Save ──────────────────────────────────────────────────────────────────

  async function saveConfig() {
    saving.value  = true
    saveMsg.value = ''
    try {
      const payload = JSON.stringify({
        preferences: { ...prefs },
        zone_filter: { include_types: [...zoneFilter.include_types] },
      }, null, 2)

      await exec(`echo '${payload.replace(/'/g, "'\\''")}' > '${configFile}'`)
      saveMsg.value = 'saved'
    } catch (e) {
      saveMsg.value = 'error'
    } finally {
      saving.value = false
      setTimeout(() => { saveMsg.value = '' }, 2000)
    }
  }

  // ── Threshold helpers ─────────────────────────────────────────────────────

  // Convert millidegrees to °C for display, 0 means "use default"
  function mcToC(mc) { return mc > 0 ? mc / 1000 : 0 }
  function cToMc(c)  { return c  > 0 ? Math.round(c * 1000) : 0 }

  const DEFAULT_THRESHOLDS = { cool: 40, warm: 50, hot: 60, critical: 70 }

  return {
    loaded, saving, saveMsg, prefs, zoneFilter,
    loadConfig, saveConfig,
    mcToC, cToMc, DEFAULT_THRESHOLDS,
  }
})
