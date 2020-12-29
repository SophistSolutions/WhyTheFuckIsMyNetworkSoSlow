<template>
  <v-app-bar app color="primary" dark>
    <v-app-bar-nav-icon></v-app-bar-nav-icon>
    <div class="d-flex align-center">
      <div>WhyTheFuckIsMyNetworkSoSlow</div>
    </div>

    <v-spacer></v-spacer>

    <v-breadcrumbs :items="this.$route.meta.breadcrumbs">
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
          <v-btn to="/about" text>
            <span class="mr-2">about</span>
          </v-btn>
        </v-list-item>
        <v-spacer></v-spacer>
        <v-list-item>
          <v-btn to="/devices" text>
            <span class="mr-2">devices</span>
          </v-btn>
        </v-list-item>
        <v-list-item>
          <v-btn to="/networks" text>
            <span class="mr-2">networks</span>
          </v-btn>
        </v-list-item>
        <v-divider> </v-divider>
        <v-list-item>
          <v-btn to="/devices-DEPRECATED" text>
            <span class="mr-2">devices-DEPRECATED</span>
          </v-btn>
        </v-list-item>
        <v-list-item>
          <v-btn to="/networks-DEPRECATED" text>
            <span class="mr-2">networks-DEPRECATED</span>
          </v-btn>
        </v-list-item>
        <v-spacer></v-spacer>
      </v-list>
    </v-menu>
  </v-app-bar>
</template>

<script lang="ts">
import { Component, Prop, Vue, Watch } from "vue-property-decorator";

@Component({
  name: "AppBar",
})
export default class AppBar extends Vue {
  private get search() {
    return this.$store.getters.getSearchString;
  }
  private set search(value) {
    this.$store.commit("setSearchString", value);
  }
}
</script>

<style scoped lang="scss"></style>
