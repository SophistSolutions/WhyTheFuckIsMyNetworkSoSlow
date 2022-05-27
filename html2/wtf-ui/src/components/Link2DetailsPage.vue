<script setup lang="ts">
import { defineProps } from 'vue';

const props = defineProps({
    link: { type: String, required: false, default: null },
})
</script>

<style scoped lang="scss">
</style>

<template>
    <a :href="link" target="_blank"><img src="images/as-new-page-link.png" title="View in new page" width=19
            style="padding-top: 5px; float:right" /></a>
</template>
