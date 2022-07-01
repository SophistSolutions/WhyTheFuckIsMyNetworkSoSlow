<script setup lang="ts">
import { defineProps } from 'vue';

const props = defineProps({
    link: { type: String, required: false, default: undefined },
})
</script>

<template>
    <a :href="props.link" target="_blank"><img src="images/as-new-page-link.png" title="View details in new page"
            width=19 /></a>
</template>
