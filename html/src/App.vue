<template>
  <v-app>
    <v-app-bar app color="primary" dark>
      <v-app-bar-nav-icon></v-app-bar-nav-icon>
      <div class="d-flex align-center">
        <div>WhyTheFuckIsMyNetworkSoSlow</div>
      </div>

      <v-spacer></v-spacer>

      <v-breadcrumbs :items="breadcrumbs">
        <template v-slot:divider>
          <v-icon>mdi-chevron-right</v-icon>
        </template>
        <template v-slot:item="{ item }">
          <v-breadcrumbs-item :href="item.href" :disabled="item.disabled">
            {{ item.text.toUpperCase() }}
          </v-breadcrumbs-item>
        </template>
      </v-breadcrumbs>

      <v-spacer></v-spacer>

      <v-btn icon>
        <!-- @todo add search function -->
        <v-icon>mdi-magnify</v-icon>
        <v-text-field
          v-model="search"
          append-icon="mdi-magnify"
          label="Search"
          single-line
          hide-details
        ></v-text-field>
      </v-btn>

      <v-menu bottom left>
        <template v-slot:activator="{ on }">
          <v-btn icon color="white" v-on="on">
            <v-icon>mdi-dots-vertical</v-icon>
          </v-btn>
        </template>

        <v-list>
          <v-list-item>
            <v-btn to="/" text>
              <span class="mr-2">Home</span>
            </v-btn>
          </v-list-item>
          <v-divider> </v-divider>
          <v-list-item>
            <v-btn to="about" text>
              <span class="mr-2">about</span>
            </v-btn>
          </v-list-item>
          <v-spacer></v-spacer>
          <v-list-item>
            <v-btn to="devices" text>
              <span class="mr-2">devices</span>
            </v-btn>
          </v-list-item>
          <v-list-item>
            <v-btn to="networks" text>
              <span class="mr-2">networks</span>
            </v-btn>
          </v-list-item>
          <v-divider> </v-divider>
          <v-list-item>
            <v-btn to="devices-DEPRECATED" text>
              <span class="mr-2">devices-DEPRECATED</span>
            </v-btn>
          </v-list-item>
          <v-list-item>
            <v-btn to="networks-DEPRECATED" text>
              <span class="mr-2">networks-DEPRECATED</span>
            </v-btn>
          </v-list-item>
          <v-spacer></v-spacer>
        </v-list>
      </v-menu>
    </v-app-bar>

    <v-content>
      <router-view />
    </v-content>
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
