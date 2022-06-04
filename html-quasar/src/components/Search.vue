<script setup lang="ts">


import { defineProps, watch } from 'vue';

const props = defineProps({
  searchFor: { type: String, required: false },
})

// see https://stackoverflow.com/questions/43933405/can-i-disable-certain-vue-warnings-for-a-specific-component
var searchForBWA: string = "";

watch([() => props.searchFor], () => searchForBWA = props.searchFor == null ? "" : props.searchFor)
</script>

<style scoped lang="scss">
</style>

<template>
  <q-icon name="mdi-magnify" left="true" />
  <q-input v-model="searchForBWA" label="Search" @keyup="$emit('update:searchFor', searchForBWA)" />
</template>
