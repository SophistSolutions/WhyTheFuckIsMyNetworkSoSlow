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

  data: () => ({
    breadcrumbs: [
      {
        text: "Home",
        disabled: true,
        to: "home",
      },
    ],
    search: "",
  }),

  watch: {
    $route() {
      this.breadcrumbs = this.$route.meta.breadcrumbs;
    },
    search() {
      this.$store.commit("setSearchString", this.search);
    },
  },
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
a.v-breadcrumbs__item {
  color: white !important;
  text-decoration: underline !important;
}
</style>
