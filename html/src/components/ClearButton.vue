<script setup lang="ts">
import { defineEmits } from 'vue';

const emit = defineEmits(['click'])

const onClick = () => {
  emit('click')
}
</script>

<template>
  <q-btn fab-mini dense dark size="sm" @click="onClick">
    <q-icon dense dark size="sm" name="mdi-close" />
  </q-btn>
</template>

<style scoped lang="scss">
q-btn {
  float: right;
}
</style>
