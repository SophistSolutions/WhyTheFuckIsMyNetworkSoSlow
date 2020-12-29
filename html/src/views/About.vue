<template>
  <div v-if="about" class="about">
    <app-bar />
    <br />
    <h1>About 'Why The Fuck is My Network So Slow'</h1>
    <br />
    <div>App Version: {{ about.applicationVersion }}</div>
    <div>App Running on Operating System: {{ about.operatingSystem.fullVersionedName }}</div>

    <br />

    <div>
      Written by
      <ul>
        <li>
          Lewis G. Pringle, Jr. <a href="https://www.linkedin.com/in/lewispringle/">LinkedIn</a> |
          <a href="https://github.com/LewisPringle">GitHub</a>
        </li>
        <li>
          Robert Lemos Pringle
          <a href="https://github.com/robertpringle">GitHub</a>
        </li>
      </ul>
    </div>

    <div>
      Report issues at
      <a href="https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues"
        >github issues</a
      >
    </div>
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
