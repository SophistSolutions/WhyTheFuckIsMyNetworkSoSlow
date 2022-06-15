<script setup lang="ts">
import { onMounted, defineProps, watch } from 'vue';

const props = defineProps({
    message: { type: String, required: true },
    link: { type: String, required: false, default: undefined },
    popupTitle: { type: String, required: false, default: undefined },
})

function onChange() {
    useTitle = props.popupTitle == undefined ? props.message : props.popupTitle;
    if (useTitle === undefined) {
        useTitle = "";
    }
}

onMounted(() => {
    onChange()
})

watch([() => props.message, () => props.popupTitle], onChange)

var useTitle: string = "";
</script>

<template>
    <span>
        <span v-if="!link" :title="useTitle">{{ message }}</span>
        <a v-if="link" :href=link :title="useTitle">{{ message }}</a>
    </span>
</template>
