
<script setup lang="ts">
import { onMounted} from 'vue';
import { useStore } from 'vuex'
import { duration } from 'moment';
import prettyBytes from 'pretty-bytes';

import { IAbout } from "@/models/IAbout";

import AppBar from "@/components/AppBar.vue";

var polling: undefined | number = undefined;

const store = useStore()

function about(): IAbout {
  return store.getters.getAboutInfo;
}

function pollData() {
  // first time check quickly, then more gradually
  store.dispatch("fetchAboutInfo");
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    store.dispatch("fetchAboutInfo");
  }, 10 * 1000);
}

onMounted(() => {
  pollData()
})
</script>

<template>
  <div class="about">
    <app-bar />

    <v-container class="pa-6" fluid v-if="about">
      <v-row>
        <v-col cols="12">
          <h1>About 'Why The Fuck is My Network So Slow'</h1>
        </v-col>
      </v-row>

      <v-row>
        <v-col>App</v-col>
        <v-col cols="10">
          <table id="appDataTable">
            <tr v-if="about()?.applicationVersion">
              <td>Version</td>
              <td>{{ about()?.applicationVersion }}</td>
            </tr>
            <tr v-if="about()?.serverInfo">
              <td style="vertical-align: top">Components</td>
              <td>
                <table>
                  <tr v-for="c in about()?.serverInfo?.componentVersions" :key="c.name">
                    <td>
                      <a :href="c.URL" target="_new">{{ c.name }}</a>
                    </td>
                    <td>
                      {{ c.version }}
                    </td>
                  </tr>
                </table>
              </td>
            </tr>
            <tr v-if="about()?.serverInfo">
              <td title="Average over the measurement interval for just this service process;
Units 1=1 logical core">
                CPU-Usage
              </td>
              <td>{{ about().serverInfo.currentProcess?.averageCPUTimeUsed }} CPUs</td>
            </tr>
            <tr v-if="
              about()?.serverInfo?.currentProcess &&
              about().serverInfo.currentProcess.combinedIOReadRate &&
              about().serverInfo.currentProcess.combinedIOWriteRate
            ">
              <td title="Combined I/O rate (network+disk)">IO Rate (read; write)</td>
              <td>
                {{ prettyBytes(about().serverInfo.currentProcess.combinedIOReadRate) }}/sec ;
                {{ prettyBytes(about().serverInfo.currentProcess.combinedIOWriteRate) }}/sec
              </td>
            </tr>
            <tr v-if="about()?.serverInfo?.currentProcess?.processUptime">
              <td title="How long has the service been running">Uptime</td>
              <td>
                {{ duration(about().serverInfo?.currentProcess?.processUptime).humanize() }}
              </td>
            </tr>
            <tr v-if="about()?.serverInfo?.currentProcess">
              <td title="Working set size, or RSS resident set size (how much RAM is an active use)">
                WSS
              </td>
              <td>{{ prettyBytes(about().serverInfo.currentProcess.workingOrResidentSetSize) }}</td>
            </tr>
          </table>
        </v-col>
      </v-row>

      <v-row v-if="about()?.serverInfo?.currentMachine">
        <v-col>App Running on</v-col>
        <v-col cols="10">
          <table id="appRunningOnTable">
            <tr>
              <td>OS</td>
              <td>{{ about.serverInfo?.currentMachine?.operatingSystem?.fullVersionedName }}</td>
            </tr>
            <tr>
              <td title="How long has the machine (hosting the service) been running">Uptime</td>
              <td>{{ duration(about().serverInfo?.currentMachine?.machineUptime).humanize() }}</td>
            </tr>
            <tr v-if="about().serverInfo.currentMachine.runQLength != null">
              <td
                title="How many threads in each (logical) processors Run-Q on average. 0 means no use, 1 means ALL cores fully used with no Q, and 2 means all cores fully utilized and each core with a Q length of 1">
                Run-Q</td>
              <td>{{ about().serverInfo.currentMachine.runQLength }} threads</td>
            </tr>
            <tr>
              <td title="Average over the measurement interval for the entire machine hosting the service.
Units 1=1 logical core">
                CPU-Usage
              </td>
              <td>{{ about().serverInfo.currentMachine.totalCPUUsage || "?" }} CPUs</td>
            </tr>
          </table>
        </v-col>
      </v-row>

      <v-row>
        <v-col>
          Written by
        </v-col>
        <v-col cols="10">
          <table>
            <tr>
              <td>
                Lewis G. Pringle, Jr.
              </td>
              <td>
                <a href="https://www.linkedin.com/in/lewispringle/" target="_new">LinkedIn</a> |
                <a href="https://github.com/LewisPringle" target="_new">GitHub</a>
              </td>
            </tr>
            <tr>
              <td>
                Robert Lemos Pringle
              </td>
              <td>
                <a href="https://github.com/robertpringle" target="_new">GitHub</a>
              </td>
            </tr>
          </table>
        </v-col>
      </v-row>

      <v-row>
        <v-col>
          Report issues at
        </v-col>
        <v-col cols="10">
          <a href="https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues" target="_new">github
            issues</a>
        </v-col>
      </v-row>
    </v-container>
  </div>
</template>
