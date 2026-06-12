<template>
  <div class="flex items-center gap-3 px-4 py-3">
    <div class="flex-1 min-w-0">
      <p class="text-sm font-medium">{{ title }}</p>
      <p v-if="desc" class="text-xs text-on-surface-variant mt-0.5">{{ desc }}</p>
    </div>
    <div class="shrink-0">
      <slot />
    </div>
  </div>
</template>

<script setup>
defineProps({
  title: String,
  desc:  String,
})
</script>
