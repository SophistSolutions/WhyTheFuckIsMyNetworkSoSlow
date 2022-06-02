<script setup lang="ts">
import { onMounted } from 'vue';
import { duration } from 'moment';
import prettyBytes from 'pretty-bytes';

import { IAbout } from "../models/IAbout";

import { useWTFStore } from '../stores/WTF-store'

var polling: undefined | NodeJS.Timeout = undefined;

const store = useWTFStore()

function about(): IAbout {
  return store.getAboutInfo;
}

function pollData() {
  // first time check quickly, then more gradually
  store.fetchAboutInfo();
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    store.fetchAboutInfo();
  }, 10 * 1000);
}

onMounted(() => {
  pollData()
})
</script>

<template>
  <q-page class="col q-pa-md q-gutter-md" v-if="about">

    <div class="row text-h5">
      About 'Why The Fuck is My Network So Slow'
    </div>

    <!--App Description Overview-->
    <q-card>
      <q-card-section>
        Why The Fuck is My Network So Slow monitors your local network, and tracks over time what devices are on the
        network, and what traffic they generate.
        It (will soon) allow you to see what is normal behavior on your network, and notify you of interesting
        abberations, to help see
        why your network maybe sometimes slow.
      </q-card-section>
    </q-card>

    <!--App Stats-->
    <q-card>
      <q-card-section>
        <div class="row">
          <div class="col-2">WTF App</div>
          <div class="col-10">
            <table>
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
          </div>
        </div>
      </q-card-section>
    </q-card>


    <!--App Running on-->
    <q-card>
      <q-card-section>
        <div >
          <div class="row" v-if="about()?.serverInfo?.currentMachine">
            <div class="col-2">WTF Running on</div>
            <div class="col-10">
              <table id="appRunningOnTable">
                <tr>
                  <td>OS</td>
                  <td>{{ about().serverInfo?.currentMachine?.operatingSystem?.fullVersionedName }}</td>
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
            </div>
          </div>
        </div>
      </q-card-section>
    </q-card>


    <!--Written by-->

    <q-card>
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
    <q-card>
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
  </q-page>
</template>
