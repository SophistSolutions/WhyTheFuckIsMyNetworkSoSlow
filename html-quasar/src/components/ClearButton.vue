<style scoped lang="scss">
v-btn {
  float: right;
}
</style>

<template>
  <v-btn class="mx-4" fab dark x-small v-on:click="$emit('click')">
    <q-icon dark x-small>
      mdi-close
    </q-icon>
  </v-btn>
</template>