<template>
  <v-app>
    <v-main>
      <router-view />
    </v-main>
  </v-app>
</template>

<script lang="ts">
import Vue from "vue";

export default Vue.extend({
  name: "App",
});

Vue.directive("click-outside", {
  // Note - all these casts needed because vm, and event not declared as part of DirectiveFunction
  // 'event' makes sense since we are adding it, but vm/expression we count on being there, and not sure why its
  // missing from declaration -- LGP 2020-08-17
  bind(el: HTMLElement, binding: any, vnode: any, oldVnode: any) {
    (this as any).event = (event: any) => (this as any).vm.$emit((this as any).expression, event);
    this.el.addEventListener("click", (this as any).stopProp);
    document.body.addEventListener("click", (this as any).event);
  },
  unbind(el: HTMLElement, binding: any, vnode: any, oldVnode: any) {
    this.el.removeEventListener("click", (this as any).stopProp);
    document.body.removeEventListener("click", (this as any).event);
  },
  stopProp(event: any) {
    event.stopPropagation();
  },
} as any);
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
