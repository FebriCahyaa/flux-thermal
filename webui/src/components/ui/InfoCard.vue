<template>
  <div class="rounded-2xl bg-surface-container px-4 py-3">
    <p class="text-xs text-on-surface-variant">{{ label }}</p>
    <p class="text-sm font-medium mt-0.5 truncate">{{ value || '—' }}</p>
  </div>
</template>

<script setup>
defineProps({
  label: String,
  value: String,
  icon:  String,
})
</script>
