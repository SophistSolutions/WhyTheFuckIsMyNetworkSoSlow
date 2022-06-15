<script setup lang="ts">
import { defineProps } from 'vue';

const props = defineProps({
    link: { type: String, required: false, default: undefined },
})
</script>

<style scoped lang="scss">
</style>

<template>
    <a :href="props.link" target="_blank"><img src="images/as-new-page-link.png" title="View details in new page"
            width=19 style="padding-top: 5px; float:right" /></a>
</template>
