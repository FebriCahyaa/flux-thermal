import { defineStore } from 'pinia'
import { ref } from 'vue'
import { exec } from 'kernelsu'
import * as KernelSU from '@/helpers/KernelSU'

const configPath = '/data/adb/.config/flux_thermal'
const fluxConfigPath = '/data/adb/.config/flux'
const modPath = '/data/adb/modules/flux_thermal'

export const useHomeStore = defineStore('home', () => {
  const daemonPidRaw      = ref('')
  const moduleVersion     = ref('')
  const currentProfileRaw = ref('')
  const kernelVersion     = ref('')
  const chipsetName       = ref('')
  const androidSDK        = ref('')
  const daemonStatusRaw   = ref('loading')
  const daemonError       = ref('')
  const isInitialized     = ref(false)
  const dependencyError   = ref(false)

  let profileInterval = null
  let daemonInterval  = null

  // ── Init ──────────────────────────────────────────────────────────────────

  async function initializeData() {
    if (isInitialized.value) return

    await Promise.all([
      checkDependency(),
      getServiceState(),
      getAndroidSDK(),
      getModuleVersion(),
      getCurrentProfile(),
      getKernelVersion(),
      getChipset(),
    ])

    startProfileMonitoring()
    startDaemonMonitoring()
    isInitialized.value = true
  }

  // ── Dependency check ──────────────────────────────────────────────────────

  async function checkDependency() {
    try {
      const err = await KernelSU.readFile(`${configPath}/dependency_error`)
      dependencyError.value = err.trim() === 'flux_missing'
    } catch {
      dependencyError.value = false
    }
  }

  // ── Daemon state ──────────────────────────────────────────────────────────

  function startProfileMonitoring() {
    stopProfileMonitoring()
    profileInterval = setInterval(getCurrentProfile, 1500)
  }
  function stopProfileMonitoring() {
    if (profileInterval) { clearInterval(profileInterval); profileInterval = null }
  }

  function startDaemonMonitoring() {
    stopDaemonMonitoring()
    daemonInterval = setInterval(getServiceState, 1500)
  }
  function stopDaemonMonitoring() {
    if (daemonInterval) { clearInterval(daemonInterval); daemonInterval = null }
  }

  async function getServiceState() {
    try {
      if (!KernelSU.isKSUWebUI()) throw new Error('Not in KSU WebUI')
      const { errno, stdout } = await exec('/system/bin/toybox pidof flux_thermald')
      const pid = stdout.trim()
      if (errno === 0 && pid) {
        daemonPidRaw.value    = pid
        daemonStatusRaw.value = 'running'
        daemonError.value     = ''
      } else {
        setDaemonStopped()
      }
    } catch (e) {
      setDaemonError(e.message)
    }
  }

  function setDaemonStopped() {
    daemonStatusRaw.value = 'stopped'
    daemonPidRaw.value    = ''
    daemonError.value     = ''
  }

  function setDaemonError(msg) {
    daemonStatusRaw.value = 'error'
    daemonError.value     = msg
  }

  // ── Profile ───────────────────────────────────────────────────────────────

  async function getCurrentProfile() {
    try {
      const raw = await KernelSU.readFile(`${configPath}/current_thermal_profile`)
      const t   = raw.trim()
      currentProfileRaw.value = t === '' ? 'initializing' : profileKey(t)
    } catch {
      currentProfileRaw.value = 'initializing'
    }
  }

  function profileKey(code) {
    const map = {
      '0': 'normal',
      '1': 'cool',
      '2': 'warm',
      '3': 'hot',
      '4': 'critical',
    }
    return map[String(code)] || 'unknown'
  }

  // ── Device info ───────────────────────────────────────────────────────────

  async function getAndroidSDK() {
    try {
      if (!KernelSU.isKSUWebUI()) throw new Error()
      const { stdout } = await exec('getprop ro.build.version.sdk')
      androidSDK.value = stdout.trim()
    } catch {
      androidSDK.value = 'unknown'
    }
  }

  async function getModuleVersion() {
    try {
      const content = await KernelSU.readFile(`${modPath}/module.prop`)
      const m = content.match(/^version=(.*)$/m)
      moduleVersion.value = m ? m[1].trim() : 'unknown'
    } catch {
      moduleVersion.value = 'unknown'
    }
  }

  async function getKernelVersion() {
    try {
      if (!KernelSU.isKSUWebUI()) throw new Error()
      const { stdout } = await exec('uname -r -m')
      kernelVersion.value = stdout.trim()
    } catch {
      kernelVersion.value = 'unknown'
    }
  }

  async function getChipset() {
    try {
      if (!KernelSU.isKSUWebUI()) throw new Error()
      const { stdout } = await exec('getprop ro.board.platform')
      const brand = await getChipsetBrand()
      chipsetName.value = `${brand} ${stdout.trim()}`.trim()
    } catch {
      chipsetName.value = 'unknown'
    }
  }

  async function getChipsetBrand() {
    try {
      const soc = (await KernelSU.readFile(`${fluxConfigPath}/soc_recognition`)).trim()
      return { '1':'MediaTek','2':'Snapdragon','3':'Exynos','4':'Unisoc',
               '5':'Tensor','6':'Tegra','7':'Kirin' }[soc] || ''
    } catch {
      return ''
    }
  }

  return {
    daemonPidRaw, moduleVersion, currentProfileRaw, kernelVersion,
    chipsetName, androidSDK, daemonStatusRaw, daemonError,
    isInitialized, dependencyError,
    initializeData, stopProfileMonitoring, stopDaemonMonitoring,
    getServiceState, getAndroidSDK, getModuleVersion, getCurrentProfile,
    getKernelVersion, getChipset, checkDependency,
  }
})
