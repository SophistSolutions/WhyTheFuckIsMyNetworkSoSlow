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

import { PluralizeNoun } from 'src/utils/Linguistics';


// Components
import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';
import Link2DetailsPage from '../components/Link2DetailsPage.vue';

import { useNetStateStore } from '../stores/Net-State-store'
import { INetwork } from 'src/models/network/INetwork';

const store = useNetStateStore()

const props = defineProps({
  networkInterfaceId: { type: String, required: true },
  includeLinkToDetailsPage: { type: Boolean, required: false, default: false },
  showExtraDetails: { type: Boolean, required: false, default: false },
})

defineComponent({
  components: {
  },
});

let polling: undefined | NodeJS.Timeout;

let allDevices: ComputedRef<IDevice[]> = computed(() => store.getDevices);

let currentNetworkInterface = computed<INetworkInterface | undefined>(
  () => store.getNetworkInterface(props.networkInterfaceId)
)

function doFetches() {
  store.fetchNetworkInterfaces([props.networkInterfaceId]);
};

onMounted(() => {
  // first time check immediately, then more gradually for updates
  doFetches();
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    doFetches();
  }, 30 * 1000);
})

onUnmounted(() => {
  clearInterval(polling);
})

</script>

<template>
  <div v-if="currentNetworkInterface || true" class="q-pa-sm">
    <div class="row" v-if="currentNetworkInterface?.friendlyName">
      <div class="col-3">Friendly Name</div>
      <div class="col"> {{currentNetworkInterface.friendlyName}} </div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.type">
      <div class="col-3">Type</div>
      <div class="col"> {{currentNetworkInterface.type}} </div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.description">
      <div class="col-3">Description</div>
      <div class="col"> {{currentNetworkInterface.description}} </div>
    </div>
    <div class="row">
      <div class="col-3">ID</div>
      <div class="col"> {{props.networkInterfaceId}} </div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.hardwareAddress">
      <div class="col-3">Hardware Address</div>
      <div class="col"> {{currentNetworkInterface.hardwareAddress}} </div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.boundAddressRanges">
      <div class="col-3">CIDRs</div>
      <div class="col"> {{currentNetworkInterface.boundAddressRanges.join(", ")}} </div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.boundAddresses">
      <div class="col-3">Bindings</div>
      <div class="col"> {{currentNetworkInterface.boundAddresses.join(", ")}} </div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.transmitSpeedBaud || currentNetworkInterface?.receiveLinkSpeedBaud">
      <div class="col-3">Speed (tx/rx)</div>
      <div class="col"> {{currentNetworkInterface.transmitSpeedBaud}}/{{currentNetworkInterface.receiveLinkSpeedBaud}} </div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.DNSServers">
      <div class="col-3">DNS Servers</div>
      <div class="col"> {{currentNetworkInterface.DNSServers.join(", ")}} </div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.gateways">
      <div class="col-3">Gateways</div>
      <div class="col"> {{currentNetworkInterface.gateways.join(", ")}} </div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.debugProps && props.showExtraDetails">
      <div class="col-3">DEBUG INFO</div>
      <div class="col">
        <json-viewer :value="currentNetworkInterface.debugProps" :expand-depth="0" copyable sort class="debugInfoJSONViewers" />
      </div>
    </div>
  </div>
</template>

<style scoped lang="scss">
.snapshot {
  font-style: italic;
}

.aggregatesItem {
  min-width: 10em
}
</style>