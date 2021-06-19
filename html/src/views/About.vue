<template>
  <div class="about">
    <app-bar />

    <v-container class="pa-6" fluid v-if="about">
      <v-row>
        <v-col cols="12"><h1>About 'Why The Fuck is My Network So Slow'</h1></v-col>
      </v-row>
      <v-row>
        <v-col>App</v-col>
        <v-col cols="10">
          <table>
            <tr>
              <td style="width:1in;">Version</td>
              <td>{{ about.applicationVersion }}</td>
            </tr>
            <tr>
              <td style="vertical-align: top">Components</td>
              <td>
                <table>
                  <tr v-for="c in about.serverInfo.componentVersions" :key="c.name">
                    <td>
                      <a :href="c.URL" target="_new">{{ c.name }}</a>
                    </td>
                    <td>
                      {{ c.version }}
                    </td>
                  </tr>
                </table>

                {{ about.componentVersions }}
              </td>
            </tr>
            <tr>
              <td>averageCPUTimeUsed</td>
              <td>{{ about.serverInfo.currentProcess.averageCPUTimeUsed }}</td>
            </tr>
            <tr>
              <td>combinedIOWriteRate</td>
              <td>{{ about.serverInfo.currentProcess.combinedIOWriteRate }}</td>
            </tr>
            <tr>
              <td>processUptime</td>
              <td>{{ about.serverInfo.currentProcess.processUptime }}</td>
            </tr>
            <tr>
              <td>workingOrResidentSetSize</td>
              <td>{{ about.serverInfo.currentProcess.workingOrResidentSetSize }}</td>
            </tr>
          </table>
        </v-col>
      </v-row>
      <v-row>
        <v-col>App Running on</v-col>
        <v-col cols="10">
          <table>
            <tr>
              <td style="width:1in;">OS</td>
              <td>{{ about.serverInfo.currentMachine.operatingSystem.fullVersionedName }}</td>
            </tr>
            <tr>
              <td>Uptime</td>
              <td>{{ about.serverInfo.currentMachine.machineUptime }}</td>
            </tr>
            <tr>
              <td>runQLength</td>
              <td>{{ about.serverInfo.currentMachine.runQLength }}</td>
            </tr>
            <tr>
              <td>totalCPUUsage</td>
              <td>{{ about.serverInfo.currentMachine.totalCPUUsage }}</td>
            </tr>
          </table>
        </v-col>
      </v-row>
      <v-row>
        <v-col>
          Written by
        </v-col>
        <v-col cols="10">
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
        <v-col cols="10">
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
