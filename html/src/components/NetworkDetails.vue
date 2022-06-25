<script setup lang="ts">
import { defineProps, defineComponent, onMounted, onUnmounted, ref, computed, ComputedRef } from 'vue';

import JsonViewer from 'vue-json-viewer';
import moment from 'moment';

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
import Link2DetailsPage from '../components/Link2DetailsPage.vue';

import { useNetStateStore } from '../stores/Net-State-store'
import { INetwork } from 'src/models/network/INetwork';

const store = useNetStateStore()

const props = defineProps({
  networkId: { type: String, required: true },
  includeLinkToDetailsPage: { type: Boolean, required: false, default: false },
})

defineComponent({
  components: {
    ReadOnlyTextWithHover,
    JsonViewer,
    Link2DetailsPage,
  },
});

let polling: undefined | NodeJS.Timeout;

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
  <div v-if="currentNetwork" class="q-pa-sm">
    <Link2DetailsPage :link="'/#/network/' + currentNetwork.id" v-if="props.includeLinkToDetailsPage" />

    <div class="row">
      <div class="col-3"> Name </div>
      <div class="col"> {{ GetNetworkName(currentNetwork) }} </div>
    </div>
    <div class="row">
      <div class="col-3"> ID </div>
      <div class="col"> {{ currentNetwork.id }} <span class="snapshot" v-if="currentNetwork.historicalSnapshot == true">{snapshot}</span></div>
    </div>
    <div class="row">
      <div class="col-3"> Friendly Name </div>
      <div class="col"> {{ currentNetwork.friendlyName }} </div>
    </div>
    <div class="row" v-if="currentNetwork.lastSeenAt">
      <div class="col-3"> Last Seen </div>
      <div class="col"> {{ moment(currentNetwork.lastSeenAt).fromNow() }} </div>
    </div>
    <div class="row">
      <div class="col-3"> CIDRs </div>
      <div class="col"> {{ GetNetworkCIDRs(currentNetwork) }} </div>
    </div>
    <div class="row"  v-if="currentNetwork.DNSServers && currentNetwork.DNSServers.length">
      <div class="col-3"> DNS Servers </div>
      <div class="col"> {{ currentNetwork.DNSServers.join(", ") }} </div>
    </div>
    <div class="row"  v-if="currentNetwork.gateways && currentNetwork.gateways.length">
      <div class="col-3"> Gateways </div>
      <div class="col"> {{ currentNetwork.gateways.join(", ") }} </div>
    </div>
    <div class="row"  v-if="currentNetwork.geographicLocation">
      <div class="col-3"> Geographic Location </div>
      <div class="col"> {{ FormatLocation(currentNetwork.geographicLocation) }} </div>
    </div>
    <div class="row"  v-if="currentNetwork.internetServiceProvider">
      <div class="col-3"> Internet Service Provider </div>
      <div class="col"> {{ currentNetwork.internetServiceProvider.name }} </div>
    </div>
    <div class="row"  v-if="currentNetwork.aggregatesReversibly && currentNetwork.aggregatesReversibly.length">
      <div class="col-3"> Aggregates Reversibly </div>
      <div class="col"> <span v-for="aggregate in currentNetwork.aggregatesReversibly" v-bind:key="aggregate">
            <ReadOnlyTextWithHover :message="aggregate" :link="'/#/network/' + aggregate" />;
          </span> </div>
    </div>
    <div class="row"  v-if="currentNetwork.aggregatesIrreversibly && currentNetwork.aggregatesIrreversibly.length">
      <div class="col-3"> Aggregates Irreversibly </div>
      <div class="col"> <span v-for="aggregate in currentNetwork.aggregatesIrreversibly" v-bind:key="aggregate">
            <ReadOnlyTextWithHover :message="aggregate" :link="'/#/network/' + aggregate" />;
          </span> </div>
    </div>
    <div class="row"  v-if="currentNetwork.geographicLocation">
      <div class="col-3"> Devices </div>
      <div class="col"> <a :href="GetDevicesForNetworkLink(currentNetwork.id)">{{
              GetDeviceIDsInNetwork(currentNetwork, allDevices).length
          }}</a> </div>
    </div>
    <div class="row"  v-if="currentNetwork.attachedInterfaces">
      <div class="col-3"> ATTACHED INTERFACES </div>
      <div class="col"> <json-viewer :value="thisNetworksInterfaces" :expand-depth="0" copyable sort class="debugInfoJSONViewers" /></div>
    </div>
    <div class="row"  v-if="currentNetwork.debugProps">
      <div class="col-3"> DEBUG INFO </div>
      <div class="col">  <json-viewer :value="currentNetwork.debugProps" :expand-depth="0" copyable sort
            class="debugInfoJSONViewers" /></div>
    </div>
  </div>
</template>

<style scoped lang="scss">
.snapshot {
  font-style: italic;
}

.debugInfoJSONViewers {
  margin-right: 2em;
  background-color: transparent;
}
</style>