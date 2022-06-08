<script setup lang="ts">
import { defineProps, defineComponent, onMounted, onUnmounted, ref, computed, ComputedRef } from 'vue';

import JsonViewer from 'vue-json-viewer';
import * as moment from 'moment';

import { IDevice, INetworkAttachmentInfo } from "../models/device/IDevice";
import { INetworkInterface } from "../models/network/INetworkInterface";

import {
  FormatLocation,
  GetDeviceIDsInNetwork,
  GetDevicesForNetworkLink,
  GetNetworkCIDRs,
  GetNetworkName,
} from "../models/network/Utils";



// Components
import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';

import { useWTFStore } from '../stores/WTF-store'
import { INetwork } from 'src/models/network/INetwork';

const store = useWTFStore()

const props = defineProps({
  networkId: { type: String, required: true },
})

defineComponent({
  components: {
    ReadOnlyTextWithHover,
    JsonViewer,
  },
});

let polling: undefined | NodeJS.Timeout;
var isRescanning: boolean = false;

onMounted(() => {
  // first time check immediately, then more gradually for updates
  store.fetchNetworks([props.networkId]);
  store.fetchDevices();
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    store.fetchDevices();
    store.fetchNetworks([props.networkId]);
  }, 15 * 1000);
})

onUnmounted(() => {
  clearInterval(polling);
})

let allDevices: ComputedRef<IDevice[]> = computed(() => store.getDevices);

let currentNetwork = computed<INetwork | undefined>(
  () => store.getNetwork(props.networkId)
)

let networkInterfaces = computed<INetworkInterface[]>(
  () => store.getNetworkInterfaces
)

let thisNetworksInterfaces = computed<INetworkInterface[]>(
  () => {
    const result: INetworkInterface[] = [];
    if (currentNetwork.value) {
      currentNetwork.value.attachedInterfaces.forEach((e) => {
        let answer: INetworkInterface | undefined;
        networkInterfaces.value.forEach((ni) => {
          if (e === ni.id) {
            answer = ni;
          }
        });
        if (answer === undefined) {
          answer = { id: e };
        }
        result.push(answer);
      });
    }
    return result;
  }
)
</script>

<template>
  <div>
    <table class="detailsTable" v-bind:key="currentNetwork.id" v-if="currentNetwork">
      <tr>
        <td>Name</td>
        <td>{{ GetNetworkName(currentNetwork) }}</td>
      </tr>
      <tr>
        <td>ID</td>
        <td>
          {{ currentNetwork.id }}
          <span class="snapshot" v-if="currentNetwork.historicalSnapshot == true">{snapshot}</span>
        </td>
      </tr>
      <tr>
        <td>Friendly Name</td>
        <td>{{ currentNetwork.friendlyName }}</td>
      </tr>
      <tr v-if="currentNetwork.lastSeenAt">
        <td>Last Seen</td>
        <td>{{ moment(currentNetwork.lastSeenAt).fromNow() }}</td>
      </tr>
      <tr>
        <td>CIDRs</td>
        <td>{{ GetNetworkCIDRs(currentNetwork) }}</td>
      </tr>
      <tr v-if="currentNetwork.DNSServers && currentNetwork.DNSServers.length">
        <td>DNS Servers</td>
        <td>{{ currentNetwork.DNSServers.join(", ") }}</td>
      </tr>
      <tr v-if="currentNetwork.gateways && currentNetwork.gateways.length">
        <td>Gateways</td>
        <td>{{ currentNetwork.gateways.join(", ") }}</td>
      </tr>
      <tr v-if="currentNetwork.geographicLocation">
        <td>Geographic Location</td>
        <td>{{ FormatLocation(currentNetwork.geographicLocation) }}</td>
      </tr>
      <tr v-if="currentNetwork.internetServiceProvider">
        <td>Internet Service Provider</td>
        <td>{{ currentNetwork.internetServiceProvider.name }}</td>
      </tr>
      <tr v-if="currentNetwork.aggregatesReversibly && currentNetwork.aggregatesReversibly.length">
        <td>Aggregates Reversibly</td>
        <td>
          <span v-for="aggregate in currentNetwork.aggregatesReversibly" v-bind:key="aggregate">
            <ReadOnlyTextWithHover :message="aggregate" :link="'/#/network/' + aggregate" />;
          </span>
        </td>
      </tr>
      <tr v-if="currentNetwork.aggregatesIrreversibly && currentNetwork.aggregatesIrreversibly.length">
        <td>Aggregates Irreversibly</td>
        <td>
          <span v-for="aggregate in currentNetwork.aggregatesIrreversibly" v-bind:key="aggregate">
            <ReadOnlyTextWithHover :message="aggregate" />;
          </span>
        </td>
      </tr>
      <tr>
        <td>Devices</td>
        <td>
          <a :href="GetDevicesForNetworkLink(currentNetwork.id)">{{
              GetDeviceIDsInNetwork(currentNetwork, allDevices).length
          }}</a>
        </td>
      </tr>
      <tr v-if="currentNetwork.attachedInterfaces">
        <td>ATTACHED INTERFACES</td>
        <td>
          <json-viewer :value="thisNetworksInterfaces" :expand-depth="0" copyable sort />
        </td>
      </tr>
      <tr v-if="currentNetwork.debugProps">
        <td>DEBUG INFO</td>
        <td>
          <json-viewer :value="currentNetwork.debugProps" :expand-depth="1" copyable sort />
        </td>
      </tr>
    </table>
  </div>
</template>

<style lang="scss" scoped>
.detailsTable {
  table-layout: fixed;
}

.detailsTable td {
  padding-left: 5px;
  padding-right: 10px;
}

.snapshot {
  font-style: italic;
}
</style>
