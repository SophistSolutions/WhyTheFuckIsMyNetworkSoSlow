<script setup lang="ts">


import { onMounted, defineProps, watch } from 'vue';

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
  <v-text-field v-model="searchForBWA" prepend-icon="mdi-magnify" label="Search" single-line hide-details
    @keyup="$emit('update:searchFor', searchForBWA)"></v-text-field>
</template>
