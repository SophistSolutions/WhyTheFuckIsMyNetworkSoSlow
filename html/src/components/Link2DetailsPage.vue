<script setup lang="ts">

const props = defineProps({
    link: { type: String, required: false, default: undefined },
})
</script>

<template>
    <q-btn flat round dense title="View details in new page" :href="props.link" target="_blank">
        <img src="images/as-new-page-link.png" width=19 />
    </q-btn>
</template>
