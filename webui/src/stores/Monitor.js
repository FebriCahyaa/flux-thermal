import { defineStore } from 'pinia'
import { ref } from 'vue'
import * as KernelSU from '@/helpers/KernelSU'

const configPath = '/data/adb/.config/flux_thermal'

export const useMonitorStore = defineStore('monitor', () => {
  const zones       = ref([])
  const maxTempMc   = ref(0)
  const maxTempC    = ref(0)
  const lastUpdated = ref('')
  const loading     = ref(false)
  const error       = ref('')

  let pollInterval = null

  async function fetchThermalStatus() {
    loading.value = true
    error.value   = ''
    try {
      const raw = await KernelSU.readFile(`${configPath}/thermal_status.json`)
      const data = JSON.parse(raw)
      zones.value     = data.zones    || []
      maxTempMc.value = data.max_temp_mc || 0
      maxTempC.value  = data.max_temp_c  || 0
      lastUpdated.value = new Date().toLocaleTimeString()
    } catch (e) {
      error.value = e.message || 'Failed to read thermal status'
    } finally {
      loading.value = false
    }
  }

  function startPolling(intervalMs = 3000) {
    stopPolling()
    fetchThermalStatus()
    pollInterval = setInterval(fetchThermalStatus, intervalMs)
  }

  function stopPolling() {
    if (pollInterval) { clearInterval(pollInterval); pollInterval = null }
  }

  // Temp colour helpers
  function tempColor(tempC) {
    if (tempC >= 70) return 'text-red-500'
    if (tempC >= 60) return 'text-orange-500'
    if (tempC >= 50) return 'text-yellow-500'
    if (tempC >= 40) return 'text-lime-500'
    return 'text-green-500'
  }

  function tempBgColor(tempC) {
    if (tempC >= 70) return 'bg-red-500/10   border-red-500/30'
    if (tempC >= 60) return 'bg-orange-500/10 border-orange-500/30'
    if (tempC >= 50) return 'bg-yellow-500/10 border-yellow-500/30'
    if (tempC >= 40) return 'bg-lime-500/10   border-lime-500/30'
    return 'bg-green-500/10 border-green-500/30'
  }

  return {
    zones, maxTempMc, maxTempC, lastUpdated, loading, error,
    fetchThermalStatus, startPolling, stopPolling,
    tempColor, tempBgColor,
  }
})
