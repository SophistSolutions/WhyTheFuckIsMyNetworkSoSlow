<script setup lang="ts">
import { defineComponent, onMounted, onUnmounted } from 'vue';
import { useStore } from 'vuex'

import { IDevice } from "@/models/device/IDevice";
import { INetwork } from "@/models/network/INetwork";
import {
  FormatLocation,
  GetDeviceIDsInNetwork,
  GetNetworkLink,
  GetNetworkName,
} from "@/models/network/Utils";

// Components
import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';
import AppBar from "@/components/AppBar.vue";

defineComponent({
  name: 'HomeView',

  components: {
    AppBar,
    ReadOnlyTextWithHover,
  },
});


const store = useStore()

var polling: undefined | number = undefined;

onMounted(() => {
  store.dispatch("fetchDevices", null);
  store.dispatch("fetchAvailableNetworks");
  pollData();
})
onUnmounted(() => {
  clearInterval(polling);
})

function pollData() {
  polling = setInterval(() => {
    store.dispatch("fetchDevices", null);
    store.dispatch("fetchAvailableNetworks");
  }, 15 * 1000);
}

function networks(): INetwork[] {
  const allNetworks: INetwork[] = store.getters.getAvailableNetworks;
  const result: INetwork[] = [];
  allNetworks.forEach((i) => {
    if (i.internetServiceProvider != null || i.geographicLocation != null) {
      result.push(i);
    }
  });
  return result;
}

function networksAsDisplayed(): object[] {
  const result: object[] = [];
  networks().forEach((i: INetwork) => {
    result.push({
      id: i.id,
      name: GetNetworkName(i),
      link: GetNetworkLink(i),
      active: "true",
      internetInfo:
        (i.gateways == null ? "" : i.gateways.join(", ")) +
        (i.internetServiceProvider == null ? " " : " (" + i.internetServiceProvider.name + ")"),
      devices: GetDeviceIDsInNetwork(i, devices()).length,
      status: "healthy",
      location: FormatLocation(i.geographicLocation),
    });
  });
  result.sort((a: {id:string}, b: {id:string}) => {
    if (a.id < b.id) {
      return -1;
    }
    if (a.id > b.id) {
      return 1;
    }
    return 0;
  });
  return result;
}

function devices(): IDevice[] {
  return store.getters.getDevices;
}
</script>

<template>
  <app-bar />
  <v-container>
    <v-row class="text-center">
      <v-col class="mb-4">
        <h1 class="display-2 font-weight-bold mb-3">
          Why The Fuck Is My Network So Slow?
        </h1>
      </v-col>
    </v-row>
    <v-row>
      <v-col class="mb-4">
        <router-link to="/networks">Networks</router-link> (active + favorites)
        <ul>
          <li v-for="network in networksAsDisplayed()" :key="network.id">
            <ReadOnlyTextWithHover :message="network.name" :link="network.link" />
            <div>
              : {{ GetDeviceIDsInNetwork(network, devices()).length }}
              <router-link to="/devices">devices</router-link>, operating normally
            </div>
          </li>
        </ul>
      </v-col>
    </v-row>
  </v-container>
</template>
