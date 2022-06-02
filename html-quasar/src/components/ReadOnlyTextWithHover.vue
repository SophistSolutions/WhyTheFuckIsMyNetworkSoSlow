<script setup lang="ts">
import { onMounted, defineProps, watch } from 'vue';

const props = defineProps({
  message: { type: String, required: true },
  link: { type: String, required: false, default: null },
  popupTitle: { type: String, required: false, default: null },
})

function onChange() {
    useTitle = props.popupTitle == null ? props.message : props.popupTitle;
    if (useTitle === null) {
        useTitle = "";
    }
}

onMounted(() => {
    onChange()
})

watch ([()=>props.message,()=>props.popupTitle], onChange)

var useTitle: string = "";
</script>

<style scoped lang="scss">
</style>

<template>
    <span>
        <span v-if="!link" :title="useTitle">{{ message }}</span>
        <a v-if="link" :href=link :title="useTitle">{{ message }}</a>
    </span>
</template>
