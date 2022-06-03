<script setup lang="ts">
import { defineComponent, onMounted, onUnmounted, computed, ComputedRef } from 'vue';

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
let polling: undefined | NodeJS.Timeout;

const kRefreshFrequencyInSeconds_: number = 15;

onMounted(() => {
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

let allNetworks : ComputedRef<INetwork[]> = computed(() => store.getAvailableNetworks);
let shownNetworks : ComputedRef<INetwork[]> = computed(() => {
   const result: INetwork[] = [];
   // @todo - probaly just include 'active' and 'favorite' networks here (as it hints in UI)
  allNetworks.value.forEach((i) => {
    if (i.internetServiceProvider != null || i.geographicLocation != null) {
      result.push(i);
    }
  });
  return result;
});


interface IDisplayedNetwork {
  id: string;
  link: string | null;
  name: string;
  active: string;
  internetInfo: string;
  status: string;
  originalNetwork: INetwork;
}

let shownNetworksAsDisplayed : ComputedRef<IDisplayedNetwork[]> = computed(() => {
  const result: IDisplayedNetwork[] = [];
  shownNetworks.value.forEach((i: INetwork) => {
    result.push({
      id: i.id,
      name: GetNetworkName(i),
      link: GetNetworkLink(i),
      active: "true",
      internetInfo:
        (i.gateways ? i.gateways.join(", "): "") +
        (i.internetServiceProvider?" (" + i.internetServiceProvider.name + ")": " "),
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
});

let allDevices : ComputedRef<IDevice[]> = computed(() => store.getDevices);
</script>

<template>
  <q-page class="col q-pa-md q-gutter-md">

    <div class="row text-h5">
      Why The Fuck is My Network So Slow?
    </div>

    <q-card>
      <q-card-section>
        <div class="row">
          <div class="col">
            <router-link to="/networks">Networks</router-link> (active + favorites)
            <ul>
              <li v-for="network in shownNetworksAsDisplayed" :key="network.id">
                <ReadOnlyTextWithHover :message="network.name" :link="network.link" />
                <div v-if="network.internetInfo">
                  : {{ network.internetInfo }}
                </div>
                <div>
                  : {{ GetDeviceIDsInNetwork(network.originalNetwork, allDevices).length }}
                  <router-link to="/devices">devices</router-link>, operating normally
                </div>
              </li>
            </ul>
          </div>
        </div>
      </q-card-section>
    </q-card>
  </q-page>
</template>
