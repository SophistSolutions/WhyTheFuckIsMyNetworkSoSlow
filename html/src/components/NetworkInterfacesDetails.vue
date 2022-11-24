<script setup lang="ts">
import { PropType, defineProps, defineComponent, onMounted, onUnmounted, ref, computed, ComputedRef } from 'vue';

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
import NetworkInterfaceDetails from '../components/NetworkInterfaceDetails.vue';
import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';
import Link2DetailsPage from '../components/Link2DetailsPage.vue';

import { useNetStateStore } from '../stores/Net-State-store'
import { INetwork } from 'src/models/network/INetwork';

const store = useNetStateStore()

const props = defineProps({
  networkInterfaceIds: { type: Array as PropType<Array<string>>, required: true },
})

defineComponent({
  components: {
  },
});

let polling: undefined | NodeJS.Timeout;

let allDevices: ComputedRef<IDevice[]> = computed(() => store.getDevices);

// let currentNetworkInterface = computed<INetwork | undefined>(
//   () => undefined/*store.getNetworkInterface(props.networkInterfaceId)*/
// )

function doFetches() {
  //store.fetchNetworkInterfaces([props.networkInterfaceId]);
};

onMounted(() => {
  // first time check immediately, then more gradually for updates
  doFetches();
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    doFetches();
  }, 15 * 1000);
})

onUnmounted(() => {
  clearInterval(polling);
})

</script>

<template>
  <div v-if="true" class="q-pa-sm">
    <div class="row" v-for="interfaceId in props.networkInterfaceIds" v-bind:key="interfaceId">
      <div class="col">
        <NetworkInterfaceDetails :networkInterfaceId="interfaceId" />
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