import { defineStore } from 'pinia'
import { ref } from 'vue'
import { useI18n } from 'vue-i18n'

export const useLanguageStore = defineStore('language', () => {
  const currentLocale = ref('en')

  function setLocale(locale) {
    const { locale: i18nLocale } = useI18n()
    i18nLocale.value  = locale
    currentLocale.value = locale
    localStorage.setItem('flux_thermal_locale', locale)
  }

  function loadSavedLocale() {
    const saved = localStorage.getItem('flux_thermal_locale')
    if (saved) {
      currentLocale.value = saved
    }
  }

  return { currentLocale, setLocale, loadSavedLocale }
})
