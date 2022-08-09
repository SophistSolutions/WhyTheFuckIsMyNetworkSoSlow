<script setup lang="ts">
import { defineComponent, onMounted, onUnmounted, computed, ComputedRef } from 'vue';
import moment from 'moment';

import { IDevice } from "../models/device/IDevice";
import { INetwork } from "../models/network/INetwork";
import {
  GetDeviceIDsInNetwork,
  GetDevicesForNetworkLink,
  GetNetworkLink,
  GetNetworkName,
} from "../models/network/Utils";

import { useNetStateStore } from '../stores/Net-State-store'

// Components
import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';

defineComponent({
  components: {
    ReadOnlyTextWithHover,
  },
});

const store = useNetStateStore()
let polling: undefined | NodeJS.Timeout;

const kRefreshFrequencyInSeconds_: number = 15;

onMounted(() => {
  store.fetchActiveDevices();
  store.fetchAvailableNetworks();
  polling = setInterval(() => {
    store.fetchActiveDevices();
    store.fetchAvailableNetworks();
  }, kRefreshFrequencyInSeconds_ * 1000);
})
onUnmounted(() => {
  clearInterval(polling);
})


// return a non-negative number, with highest number most attractive to show
const kAlwaysShowMinPriority_ = 15;
const kMinHoursToBeConsideredProbablyActive_ = 1;

function showNetworkPriority_(n: INetwork) {
  // @todo - probaly just include 'active' and 'favorite' networks here (as it hints in UI)
  let r: number = 0;
  const now = moment(new Date);
  if (n.seen?.upperBound) {
    var hours = moment.duration(now.diff(n.seen.upperBound)).asHours();
    if (hours < kMinHoursToBeConsideredProbablyActive_) {
      r += 11;
    }
  }
  if (n.internetServiceProvider != null || n.geographicLocation != null) {
    r += 10;
  }
  return r;
}

let allNetworks: ComputedRef<INetwork[]> = computed(() => store.getAvailableNetworks);
let shownNetworks: ComputedRef<INetwork[]> = computed(() => {
  const result: INetwork[] = [];
  // pick out all top priority items, and any above some threshold (15)
  let priorities: Array<{ id: string, priorty: number }> = [];
  allNetworks.value.forEach((i) => {
    priorities.push({ id: i.id, priorty: showNetworkPriority_(i) });
  });
  priorities.sort((a: { id: string, priorty: number }, b: { id: string, priorty: number }) => {
    return b.priorty - a.priorty;
  });
  if (priorities.length != 0) {
    const maxPri = priorities[0].priorty;
    priorities = priorities.filter(item => item.priorty == maxPri || item.priorty >= kAlwaysShowMinPriority_);
  }
  priorities.forEach((i: { id: string, priorty: number }) => {
    result.push(allNetworks.value.find(x => x.id == i.id));
  });
  return result;
});

interface IDisplayedNetwork {
  id: string;
  showPriority: number;
  link: string | null;
  name: string;
  active: string;
  internetInfo: string;
  status: string;
  originalNetwork: INetwork;
}

let shownNetworksAsDisplayed: ComputedRef<IDisplayedNetwork[]> = computed(() => {
  const result: IDisplayedNetwork[] = [];
  shownNetworks.value.forEach((i: INetwork) => {
    result.push({
      id: i.id,
      name: GetNetworkName(i),
      link: GetNetworkLink(i),
      active: "true",
      internetInfo:
        (i.gateways ? i.gateways.join(", ") : "") +
        (i.internetServiceProvider ? " (" + i.internetServiceProvider.name + ")" : " "),
      status: "healthy",
      originalNetwork: i
    });
  });
  return result;
});

let allDevices: ComputedRef<IDevice[]> = computed(() => store.getDevices);
</script>

<template>
  <q-page padding>

    <div class="row text-h4 text-center q-mb-md">
      <div class="col">
        Why The Fuck is My Network So Slow?
      </div>
    </div>

    <div class="justify-center row ">
      <q-card class="col-11 pageCard">
        <q-card-section>
          <div class="row">
            <div class="col">
              <router-link to="/networks">Networks</router-link> (active + favorites)
              <ul>
                <li v-for="network in shownNetworksAsDisplayed" :key="network.id" class="q-mb-md">
                  <ReadOnlyTextWithHover :message="network.name" :link="network.link" />
                  <div v-if="network.internetInfo">
                    : {{ network.internetInfo }}
                  </div>
                  <div>
                    : Last Seen: {{ moment(network.originalNetwork.seen?.upperBound).fromNow() }}
                  </div>
                  <div>
                    : {{ GetDeviceIDsInNetwork(network.originalNetwork, allDevices).length }}
                    <a :href="GetDevicesForNetworkLink(network.originalNetwork.id)">devices</a>
                    , operating normally
                  </div>
                </li>
              </ul>
            </div>
          </div>
        </q-card-section>
      </q-card>
    </div>

  </q-page>
</template>
