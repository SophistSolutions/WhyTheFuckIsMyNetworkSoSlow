<template>
  <v-app>
    <v-main>
      <router-view />
    </v-main>
  </v-app>
</template>

<script lang="ts">
import { defineComponent } from 'vue'

export default defineComponent({
  name: 'App',

  data () {
    return {
      //
    }
  }
})
</script>

<style lang="scss">
// figure out why I cannot migrate this worarkound to the app-bar component?
// also, find a better way than using !important
// --LGP 2020-12-29
a.v-breadcrumbs__item {
  color: white !important;
  text-decoration: underline !important;
}
</style>
