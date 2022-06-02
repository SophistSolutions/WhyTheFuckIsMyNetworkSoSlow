<script setup lang="ts">
import { defineComponent, onMounted, onUnmounted } from 'vue';

import { IDevice } from "../models/device/IDevice";
import { INetwork } from "../models/network/INetwork";
import {
  FormatLocation,
  GetDeviceIDsInNetwork,
  GetNetworkLink,
  GetNetworkName,
} from "../models/network/Utils";
import { useWTFStore } from '../stores/WTF-store'

// Components
import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';

defineComponent({
  components: {
    ReadOnlyTextWithHover,
  },
});

const store = useWTFStore()
let polling:  undefined | NodeJS.Timeout;

const kRefreshFrequencyInSeconds_: number = 15;

onMounted(() => {
  console.log('got to mounted')
  store.fetchDevices();
  store.fetchAvailableNetworks();
  polling = setInterval(() => {
    store.fetchDevices();
    store.fetchAvailableNetworks();
  }, kRefreshFrequencyInSeconds_ * 1000);
})
onUnmounted(() => {
  clearInterval(polling);
})


function networks(): INetwork[] {
  const allNetworks: INetwork[] = store.getAvailableNetworks;
  const result: INetwork[] = [];
  allNetworks.forEach((i) => {
    if (i.internetServiceProvider != null || i.geographicLocation != null) {
      result.push(i);
    }
  });
  return result;
}
interface IDisplayedNetwork   {
  id: string;
  link: string | null;
  name: string;
  active: string;
  internetInfo: string;
  status: string;
  originalNetwork: INetwork;
}

function networksAsDisplayed(): IDisplayedNetwork[] {
  const result: IDisplayedNetwork[] = [];
  networks().forEach((i: INetwork) => {
    result.push({
      id: i.id,
      name: GetNetworkName(i),
      link: GetNetworkLink(i),
      active: "true",
      internetInfo:
        (i.gateways == null ? "" : i.gateways.join(", ")) +
        (i.internetServiceProvider == null ? " " : " (" + i.internetServiceProvider.name + ")"),
      status: "healthy",
      originalNetwork: i
    });
  });
  result.sort((a: IDisplayedNetwork, b: IDisplayedNetwork) => {
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
  return store.getDevices;
}
</script>

<template>
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
              : {{ GetDeviceIDsInNetwork(network.originalNetwork, devices()).length }}
              <router-link to="/devices">devices</router-link>, operating normally
            </div>
          </li>
        </ul>
      </v-col>
    </v-row>
  </v-container>
</template>
