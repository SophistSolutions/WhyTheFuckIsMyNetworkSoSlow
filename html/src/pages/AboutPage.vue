<script setup lang="ts">
import { onMounted, onUnmounted, computed } from 'vue';
import { useQuasar } from 'quasar';
import moment from 'moment';
import prettyBytes from 'pretty-bytes';

import { VUE_VERSION } from '../config/config';
import { IAbout, IComponent } from "../models/IAbout";
import { useNetStateStore } from '../stores/Net-State-store'

let polling: undefined | NodeJS.Timeout;
const $q = useQuasar()

const kUIComponents: IComponent[] = [
  { name: "Vue ", version: VUE_VERSION, URL: "https://vuejs.org/" },
  { name: "Quasar ", version: $q.version, URL: "https://quasar.dev/" },
];

const kRefreshFrequencyInSeconds_: number = 10;

const store = useNetStateStore()

let aboutData = computed(() => store.getAboutInfo);

onMounted(() => {
  // first time check quickly, then more gradually
  store.fetchAboutInfo();
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    store.fetchAboutInfo();
  }, kRefreshFrequencyInSeconds_ * 1000);
})
onUnmounted(() => {
  clearInterval(polling);
})
</script>

<template>
  <q-page class="q-pa-md " v-if="aboutData">

    <div class="row text-h5">
      <div class="col">
        About 'Why The Fuck is My Network So Slow'
      </div>
    </div>

    <div class="row q-pa-md justify-center">

      <!--App Description Overview-->
      <q-card class="pageCard col-11 ">
        <q-card-section>
          Why The Fuck is My Network So Slow monitors your local network, and tracks over time what devices are on the
          network, and what traffic those devices generate. It also monitors the 'speed' of your various network links.
          It allows you to see what is normal behavior on your network, and notify you of interesting
          abberations, to help see why your network maybe sometimes slow.
        </q-card-section>
        <q-card-section>
          Multiple WTF instances can be setup on different machines on a network to share information with each other,
          to
          help get a better multi-dimensional (and sometimes more consitent) view of your network.
        </q-card-section>
        <q-card-section style="margin-left: 1em; padding: 0; padding-bottom: 0; font-style: italic; font-weight: bold;">
          (vision, not all implemented)
        </q-card-section>
      </q-card>

      <!--App Stats-->
      <q-card class="pageCard col-11">
        <q-card-section>
          <div class="row">
            <div class="col-2">WTF App</div>
            <div class="col-10">
              <table>
                <tr v-if="aboutData">
                  <td>Version</td>
                  <td>{{ aboutData.applicationVersion }}</td>
                </tr>
                <tr v-if="aboutData">
                  <td style="vertical-align: top">Components</td>
                  <td>
                    <table>
                      <tr v-for="c in aboutData.serverInfo.componentVersions.concat(kUIComponents)" :key="c.name">
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
                <tr v-if="aboutData">
                  <td title="Average CPU usage of the WTF (server app process) over the last 30 seconds;
Units 1=1 logical core">
                    CPU-Usage
                  </td>
                  <td>{{ aboutData.serverInfo.currentProcess.averageCPUTimeUsed || "?" }} CPUs</td>
                </tr>
                <tr v-if="
                  aboutData &&
                  aboutData.serverInfo.currentProcess.combinedIOReadRate &&
                  aboutData.serverInfo.currentProcess.combinedIOWriteRate
                ">
                  <td title="Combined I/O rate (network+disk)">IO Rate (read; write)</td>
                  <td>
                    {{ prettyBytes(aboutData.serverInfo.currentProcess.combinedIOReadRate) }}/sec ;
                    {{ prettyBytes(aboutData.serverInfo.currentProcess.combinedIOWriteRate) }}/sec
                  </td>
                </tr>
                <tr v-if="aboutData && aboutData.serverInfo.currentProcess.processUptime">
                  <td title="How long has the service been running">Uptime</td>
                  <td>
                    {{ moment.duration(aboutData.serverInfo?.currentProcess?.processUptime).humanize() }}
                  </td>
                </tr>
                <tr v-if="aboutData">
                  <td title="Working set size, or RSS resident set size (how much RAM is an active use)">
                    WSS
                  </td>
                  <td>{{ prettyBytes(aboutData.serverInfo.currentProcess.workingOrResidentSetSize) }}</td>
                </tr>
              </table>
            </div>
          </div>
        </q-card-section>
      </q-card>

      <!--App Running on-->
      <q-card class="pageCard col-11">
        <q-card-section>
          <div>
            <div class="row" v-if="aboutData">
              <div class="col-2">WTF Running on</div>
              <div class="col-10">
                <table>
                  <tr>
                    <td>OS</td>
                    <td>{{ aboutData.serverInfo.currentMachine.operatingSystem.fullVersionedName }}</td>
                  </tr>
                  <tr>
                    <td title="How long has the machine (hosting the service) been running">Uptime</td>
                    <td>{{ moment.duration(aboutData.serverInfo.currentMachine.machineUptime).humanize() }}</td>
                  </tr>
                  <tr v-if="aboutData.serverInfo.currentMachine.runQLength">
                    <td
                      title="How many threads in each (logical) processors Run-Q on average. 0 means no use, 1 means ALL cores fully used with no Q, and 2 means all cores fully utilized and each core with a Q length of 1">
                      Run-Q</td>
                    <td>{{ aboutData.serverInfo.currentMachine.runQLength }} threads</td>
                  </tr>
                  <tr>
                    <td title="Average CPU usage for the last 30 seconds for the entire machine hosting the service.
Units 1=1 logical core">
                      CPU-Usage
                    </td>
                    <td>{{ aboutData.serverInfo.currentMachine.totalCPUUsage || "?" }} CPUs</td>
                  </tr>
                </table>
              </div>
            </div>
          </div>
        </q-card-section>
      </q-card>

      <!--Written by-->
      <q-card class="pageCard col-11">
        <q-card-section>
          <div class="row">
            <div class="col-2">
              Written by
            </div>
            <div class="col-10">
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
            </div>
          </div>
        </q-card-section>
      </q-card>

      <!--Report issues at-->
      <q-card class="pageCard q-mt-md col-11">
        <q-card-section>
          <div class="row">
            <div class="col-2">
              Report issues at
            </div>
            <div class="col-10">
              <a href="https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues" target="_new">github
                issues</a>
            </div>
          </div>
        </q-card-section>
      </q-card>

    </div>

  </q-page>
</template>

<style lang="scss" scoped>
.pageCard {
  margin-bottom: 1.2em;
  max-width: 750px;
}

.q-card__section {
  padding-top: 8px;
  padding-bottom: 8px;

}
</style>
