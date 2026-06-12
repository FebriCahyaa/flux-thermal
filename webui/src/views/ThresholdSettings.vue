<template>
  <div class="page flex flex-col overflow-y-auto">
    <div class="h-[var(--window-inset-top,0px)] shrink-0" />

    <!-- Header with back button -->
    <div class="px-4 pt-4 pb-2 flex items-center gap-3 shrink-0">
      <button @click="$router.back()"
              class="p-2 -ml-2 rounded-full hover:bg-surface-container active:scale-95 transition-transform">
        <ArrowLeftIcon class="w-5 h-5 text-on-background" />
      </button>
      <div>
        <h1 class="text-xl font-semibold text-on-background">{{ t('settings.thresholds') }}</h1>
        <p class="text-xs text-on-surface-variant">{{ t('settings.thresholds_desc') }}</p>
      </div>
    </div>

    <!-- Profile reference card -->
    <div class="mx-4 mb-3 rounded-2xl bg-surface-container p-4 shrink-0">
      <p class="text-xs text-on-surface-variant mb-2 font-medium uppercase tracking-wider">Profile Reference</p>
      <div class="space-y-1.5">
        <ProfileRefRow label="Normal"   range="< 40°C (default)" color="text-green-500" />
        <ProfileRefRow label="Cool"     range="40–50°C (default)" color="text-sky-400"  />
        <ProfileRefRow label="Warm"     range="50–60°C (default)" color="text-yellow-400" />
        <ProfileRefRow label="Hot"      range="60–70°C (default)" color="text-orange-500" />
        <ProfileRefRow label="Critical" range="≥ 70°C (default)"  color="text-red-500"  />
      </div>
    </div>

    <!-- Threshold inputs -->
    <div class="mx-4 rounded-2xl bg-surface-container divide-y divide-outline-variant/30 overflow-hidden mb-3 shrink-0">
      <ThresholdInput
        :label="t('settings.threshold_cool')"
        :placeholder="'40 (default)'"
        v-model="coolC"
        @input="markDirty" />
      <ThresholdInput
        :label="t('settings.threshold_warm')"
        :placeholder="'50 (default)'"
        v-model="warmC"
        @input="markDirty" />
      <ThresholdInput
        :label="t('settings.threshold_hot')"
        :placeholder="'60 (default)'"
        v-model="hotC"
        @input="markDirty" />
      <ThresholdInput
        :label="t('settings.threshold_critical')"
        :placeholder="'70 (default)'"
        v-model="critC"
        @input="markDirty" />
    </div>

    <!-- Zone filter -->
    <div class="mx-4 rounded-2xl bg-surface-container p-4 mb-3 shrink-0">
      <p class="text-sm font-medium mb-1">{{ t('settings.zone_filter') }}</p>
      <p class="text-xs text-on-surface-variant mb-3">{{ t('settings.zone_filter_desc') }}</p>
      <div class="flex flex-wrap gap-2">
        <button v-for="type in ALL_ZONE_TYPES" :key="type"
                @click="toggleZoneType(type)"
                :class="['px-3 py-1 rounded-full text-xs font-medium transition-colors',
                         isZoneTypeActive(type)
                           ? 'bg-primary text-on-primary'
                           : 'bg-surface-variant text-on-surface-variant']">
          {{ type }}
        </button>
      </div>
    </div>

    <!-- Save button -->
    <Transition name="fade">
      <div v-if="isDirty" class="fixed bottom-20 inset-x-4 z-20">
        <button @click="handleSave"
                :disabled="cfg.saving"
                class="w-full rounded-2xl bg-primary text-on-primary py-3.5 text-sm font-semibold
                       active:scale-95 transition-transform disabled:opacity-60 shadow-lg shadow-primary/30">
          <span v-if="cfg.saving">Saving…</span>
          <span v-else>{{ t('settings.save') }}</span>
        </button>
      </div>
    </Transition>

    <div class="h-24 shrink-0" />
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useI18n } from 'vue-i18n'
import { useThermalConfigStore } from '@/stores/ThermalConfig'
import ArrowLeftIcon from '@/components/icons/ArrowLeft.vue'

const { t } = useI18n()
const cfg   = useThermalConfigStore()

const isDirty = ref(false)
function markDirty() { isDirty.value = true }

// Convert mc ↔ °C for the input fields (0 = use default)
const coolC = computed({
  get: () => cfg.mcToC(cfg.prefs.threshold_cool_mc)    || '',
  set: v  => { cfg.prefs.threshold_cool_mc    = cfg.cToMc(Number(v) || 0) }
})
const warmC = computed({
  get: () => cfg.mcToC(cfg.prefs.threshold_warm_mc)    || '',
  set: v  => { cfg.prefs.threshold_warm_mc    = cfg.cToMc(Number(v) || 0) }
})
const hotC = computed({
  get: () => cfg.mcToC(cfg.prefs.threshold_hot_mc)     || '',
  set: v  => { cfg.prefs.threshold_hot_mc     = cfg.cToMc(Number(v) || 0) }
})
const critC = computed({
  get: () => cfg.mcToC(cfg.prefs.threshold_critical_mc)|| '',
  set: v  => { cfg.prefs.threshold_critical_mc= cfg.cToMc(Number(v) || 0) }
})

// Zone filter chips
const ALL_ZONE_TYPES = ['cpu', 'gpu', 'soc', 'skin', 'thermal', 'charger', 'battery', 'npu', 'tpu', 'isp']

function isZoneTypeActive(type) {
  return cfg.zoneFilter.include_types.includes(type)
}
function toggleZoneType(type) {
  const idx = cfg.zoneFilter.include_types.indexOf(type)
  if (idx >= 0) cfg.zoneFilter.include_types.splice(idx, 1)
  else          cfg.zoneFilter.include_types.push(type)
  markDirty()
}

async function handleSave() {
  await cfg.saveConfig()
  isDirty.value = false
}

onMounted(() => cfg.loadConfig())
</script>

<!-- Sub-components inline -->
<script>
const ProfileRefRow = {
  props: ['label', 'range', 'color'],
  template: `<div class="flex items-center gap-2">
    <span class="w-2 h-2 rounded-full shrink-0" :class="color.replace('text-','bg-')" />
    <span class="text-xs font-medium w-16" :class="color">{{ label }}</span>
    <span class="text-xs text-on-surface-variant">{{ range }}</span>
  </div>`
}

const ThresholdInput = {
  props: ['label', 'placeholder', 'modelValue'],
  emits: ['update:modelValue', 'input'],
  template: `<div class="flex items-center gap-3 px-4 py-3">
    <div class="flex-1">
      <p class="text-sm font-medium">{{ label }}</p>
      <p class="text-xs text-on-surface-variant mt-0.5">0 = use compiled-in default</p>
    </div>
    <input type="number" min="0" max="120"
           class="w-20 bg-surface-variant rounded-xl px-3 py-1.5 text-sm text-right text-on-surface
                  outline-none focus:ring-2 focus:ring-primary"
           :value="modelValue" :placeholder="placeholder"
           @input="$emit('update:modelValue', $event.target.value); $emit('input')" />
  </div>`
}
</script>

<style scoped>
.fade-enter-active, .fade-leave-active { transition: opacity 150ms, transform 150ms; }
.fade-enter-from, .fade-leave-to       { opacity: 0; transform: translateY(8px); }
</style>
