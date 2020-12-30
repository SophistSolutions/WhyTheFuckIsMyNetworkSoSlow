<template>
  <div class="about">
    <app-bar />

    <v-container class="pa-6" fluid v-if="about">
      <v-row>
        <v-col cols="12"><h1>About 'Why The Fuck is My Network So Slow'</h1></v-col>
      </v-row>
      <v-row>
        <v-col>App Version</v-col>
        <v-col cols="9">{{ about.applicationVersion }}</v-col>
      </v-row>
      <v-row>
        <v-col>App Running on OS</v-col>
        <v-col cols="9">{{ about.operatingSystem.fullVersionedName }}</v-col>
      </v-row>
      <v-row>
        <v-col>
          Written by
        </v-col>
        <v-col cols="9">
          <ul>
            <li>
              Lewis G. Pringle, Jr.
              <a href="https://www.linkedin.com/in/lewispringle/">LinkedIn</a> |
              <a href="https://github.com/LewisPringle">GitHub</a>
            </li>
            <li>
              Robert Lemos Pringle
              <a href="https://github.com/robertpringle">GitHub</a>
            </li>
          </ul>
        </v-col>
      </v-row>
      <v-row>
        <v-col>
          Report issues at
        </v-col>
        <v-col cols="9">
          <a href="https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues"
            >github issues</a
          >
        </v-col>
      </v-row>
    </v-container>
  </div>
</template>

<script lang="ts">
import { IAbout } from "@/models/IAbout";
import Vue from "vue";
import Component from "vue-class-component";

@Component({
  props: {},
  components: {
    AppBar: () => import("@/components/AppBar.vue"),
  },
})
export default class About extends Vue {
  private created() {
    this.$store.dispatch("fetchAboutInfo");
  }

  private get about(): IAbout {
    return this.$store.getters.getAboutInfo;
  }
}
</script>

<style scoped>
ul {
  list-style-type: none;
}
</style>
