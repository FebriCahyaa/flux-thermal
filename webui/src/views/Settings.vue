<template>
  <div class="page flex flex-col overflow-y-auto">
    <div class="h-[var(--window-inset-top,0px)] shrink-0" />

    <!-- Header -->
    <div class="px-4 pt-4 pb-2 shrink-0">
      <h1 class="text-2xl font-semibold text-on-background">{{ t('settings.title') }}</h1>
    </div>

    <!-- ── Section: Thermal Control ── -->
    <SectionLabel :label="t('settings.section_thermal')" />

    <!-- Master enable -->
    <div class="mx-4 rounded-2xl bg-surface-container divide-y divide-outline-variant/30 overflow-hidden mb-3">
      <SettingRow
        :title="t('settings.master_enable')"
        :desc="t('settings.master_enable_desc')">
        <ToggleSwitch v-model="cfg.prefs.enabled" @change="markDirty" />
      </SettingRow>

      <SettingRow
        :title="t('settings.aggressive_mode')"
        :desc="t('settings.aggressive_mode_desc')">
        <ToggleSwitch v-model="cfg.prefs.aggressive_mode" @change="markDirty" />
      </SettingRow>

      <SettingRow
        :title="t('settings.protect_battery')"
        :desc="t('settings.protect_battery_desc')">
        <ToggleSwitch v-model="cfg.prefs.protect_battery" @change="markDirty" />
      </SettingRow>

      <SettingRow
        :title="t('settings.cool_down_on_charge')"
        :desc="t('settings.cool_down_on_charge_desc')">
        <ToggleSwitch v-model="cfg.prefs.cool_down_on_charge" @change="markDirty" />
      </SettingRow>
    </div>

    <!-- ── Section: Advanced ── -->
    <SectionLabel :label="t('settings.section_advanced')" />

    <div class="mx-4 rounded-2xl bg-surface-container divide-y divide-outline-variant/30 overflow-hidden mb-3">
      <!-- Thresholds sub-page link -->
      <RouterLink to="/settings/thresholds">
        <div class="flex items-center gap-3 px-4 py-3 active:bg-surface-variant/30">
          <TuneIcon class="w-5 h-5 text-primary shrink-0" />
          <div class="flex-1">
            <p class="text-sm font-medium">{{ t('settings.thresholds') }}</p>
            <p class="text-xs text-on-surface-variant">{{ t('settings.thresholds_desc') }}</p>
          </div>
          <ChevronRightIcon class="w-4 h-4 text-on-surface-variant" />
        </div>
      </RouterLink>

      <!-- Log level sub-page link -->
      <RouterLink to="/settings/log_level">
        <div class="flex items-center gap-3 px-4 py-3 active:bg-surface-variant/30">
          <BugIcon class="w-5 h-5 text-primary shrink-0" />
          <div class="flex-1">
            <p class="text-sm font-medium">{{ t('settings.log_level') }}</p>
            <p class="text-xs text-on-surface-variant">{{ logLevelLabel }}</p>
          </div>
          <ChevronRightIcon class="w-4 h-4 text-on-surface-variant" />
        </div>
      </RouterLink>

      <!-- Language sub-page link -->
      <RouterLink to="/settings/language">
        <div class="flex items-center gap-3 px-4 py-3 active:bg-surface-variant/30">
          <LanguageIcon class="w-5 h-5 text-primary shrink-0" />
          <div class="flex-1">
            <p class="text-sm font-medium">{{ t('settings.language_selection', 'Language') }}</p>
          </div>
          <ChevronRightIcon class="w-4 h-4 text-on-surface-variant" />
        </div>
      </RouterLink>

      <!-- Save logs -->
      <button class="w-full flex items-center gap-3 px-4 py-3 active:bg-surface-variant/30"
              @click="handleSaveLogs">
        <BugIcon class="w-5 h-5 text-primary shrink-0" />
        <div class="flex-1 text-left">
          <p class="text-sm font-medium">{{ t('settings.save_logs') }}</p>
          <p class="text-xs text-on-surface-variant">{{ t('settings.save_logs_desc') }}</p>
        </div>
      </button>
    </div>

    <!-- ── Section: About ── -->
    <SectionLabel :label="t('settings.section_about')" />

    <div class="mx-4 rounded-2xl bg-surface-container divide-y divide-outline-variant/30 overflow-hidden mb-3">
      <div class="px-4 py-3">
        <p class="text-xs text-on-surface-variant">{{ t('settings.about_version') }}</p>
        <p class="text-sm font-medium">{{ moduleVersion }}</p>
      </div>
      <div class="px-4 py-3">
        <p class="text-xs text-on-surface-variant">{{ t('settings.about_author') }}</p>
        <p class="text-sm font-medium">FebriCahyaa</p>
      </div>
      <div class="px-4 py-3">
        <p class="text-xs text-on-surface-variant">{{ t('settings.about_flux_dep') }}</p>
        <p class="text-sm font-medium">Flux Tweaks (id=flux)</p>
      </div>
      <a href="https://github.com/FebriCahyaa/flux_thermal" target="_blank"
         class="flex items-center gap-3 px-4 py-3 active:bg-surface-variant/30">
        <GithubIcon class="w-5 h-5 text-primary shrink-0" />
        <span class="text-sm flex-1">{{ t('settings.about_source') }}</span>
        <OpenInNewIcon class="w-4 h-4 text-on-surface-variant" />
      </a>
    </div>

    <!-- Save button (fixed bottom) -->
    <Transition name="fade">
      <div v-if="isDirty"
           class="fixed bottom-20 inset-x-4 z-20">
        <button @click="handleSave"
                :disabled="cfg.saving"
                class="w-full rounded-2xl bg-primary text-on-primary py-3.5 text-sm font-semibold
                       active:scale-95 transition-transform disabled:opacity-60 shadow-lg shadow-primary/30">
          <span v-if="cfg.saving">Saving…</span>
          <span v-else-if="cfg.saveMsg === 'saved'">✓ {{ t('settings.saved') }}</span>
          <span v-else-if="cfg.saveMsg === 'error'" class="text-red-200">{{ t('settings.save_error') }}</span>
          <span v-else>{{ t('settings.save') }}</span>
        </button>
      </div>
    </Transition>

    <div class="shrink-0 h-24" />
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRouter } from 'vue-router'
import { exec } from 'kernelsu'

import { useThermalConfigStore } from '@/stores/ThermalConfig'
import { useHomeStore }          from '@/stores/Home'

import ToggleSwitch   from '@/components/ui/ToggleSwitch.vue'
import SettingRow     from '@/components/ui/SettingRow.vue'
import SectionLabel   from '@/components/ui/SectionLabel.vue'
import TuneIcon       from '@/components/icons/Tune.vue'
import BugIcon        from '@/components/icons/Bug.vue'
import LanguageIcon   from '@/components/icons/Language.vue'
import GithubIcon     from '@/components/icons/Github.vue'
import ChevronRightIcon from '@/components/icons/ChevronRight.vue'
import OpenInNewIcon  from '@/components/icons/OpenInNew.vue'

const { t }  = useI18n()
const cfg    = useThermalConfigStore()
const home   = useHomeStore()

const isDirty = ref(false)
const moduleVersion = computed(() => home.moduleVersion)

const LOG_LEVEL_LABELS = ['Off','Critical','Error','Warning','Info','Debug','Trace']
const logLevelLabel = computed(() => LOG_LEVEL_LABELS[cfg.prefs.log_level] || 'Info')

function markDirty() { isDirty.value = true }

async function handleSave() {
  await cfg.saveConfig()
  isDirty.value = false
}

async function handleSaveLogs() {
  try {
    const { stdout } = await exec('flux_thermal_utility save_logs')
    alert(stdout.trim() || 'Logs saved to /sdcard/Download')
  } catch (e) {
    alert('Failed: ' + e.message)
  }
}

onMounted(() => cfg.loadConfig())
</script>

<style scoped>
.fade-enter-active, .fade-leave-active { transition: opacity 150ms, transform 150ms; }
.fade-enter-from, .fade-leave-to       { opacity: 0; transform: translateY(8px); }
</style>
