<template>
  <q-btn fab-mini dense dark size="sm" v-on:click="$emit('click')">
    <q-icon dense dark size="sm" name="mdi-close" />
  </q-btn>
</template>

<style scoped lang="scss">
q-btn {
  float: right;
}
</style>
