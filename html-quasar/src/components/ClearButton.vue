<template>
  <q-btn class="mx-4" fab dark x-small v-on:click="$emit('click')">
    <q-icon dark x-small name="mdi-close"/>
  </q-btn>
</template>

<style scoped lang="scss">
q-btn {
  float: right;
}
</style>

