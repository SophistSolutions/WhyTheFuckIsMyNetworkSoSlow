
<script setup lang="ts">
import { defineComponent, defineProps, onMounted, onUnmounted, nextTick, ref, computed, ComputedRef } from 'vue';
import { useRoute, useRouter } from 'vue-router'
import * as moment from 'moment';

import { IDevice } from "../models/device/IDevice";
import {
  ComputeDeviceTypeIconURLs,
  ComputeOSIconURLList,
  ComputeServiceTypeIconURLs,
} from "../models/device/Utils";
import { INetwork } from "../models/network/INetwork";
import {
  FormatAttachedNetworkLocalAddresses,
  GetNetworkByIDQuietly,
  GetNetworkLink,
  GetNetworkName,
  GetServices,
} from "../models/network/Utils";
import { useQuasar } from 'quasar';

// Components
// import Search from '../components/Search.vue';
//import ClearButton from '../components/ClearButton.vue';
import DeviceDetails from '../components/DeviceDetails.vue';
import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';
import Link2DetailsPage from '../components/Link2DetailsPage.vue';
import FilterSummaryMessage from '../components/FilterSummaryMessage.vue';

import { useWTFStore } from '../stores/WTF-store'
const $q = useQuasar()

const store = useWTFStore()

const props = defineProps({
  selectedNetworkink: { type: String, required: false, default: null },
})


let polling: undefined | NodeJS.Timeout;

let allAvailableNetworks: ComputedRef<INetwork[]> = computed(() => store.getAvailableNetworks);

defineComponent({
  components: {
    DeviceDetails,
  },
});

onMounted(() => {
  // first time check quickly, then more gradually
  store.fetchDevices();
  store.fetchAvailableNetworks();
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    store.fetchDevices();
    store.fetchAvailableNetworks();
  }, 15 * 1000);
})

onUnmounted(() => {
  clearInterval(polling);
})

const route = useRoute()
const router = useRouter()


let device: ComputedRef<IDevice> = computed(() => {
    let r = store.getDevice(route.params.id);
    if (r == null) {
      r = { id: "INVALID", attachedNetworks: {}, name: "INVALID", type: null as any };
    }
    return r;
  }
);
</script>

<style lang="scss">
.detailsSection {
  margin-left: 2em;
}
</style>

<template>
  <v-container>
    <app-bar />
    <v-card class="deviceListCard">
      <v-card-title>
        Device {{ device == null ? "loading..." : '"' + device.name + '"' }}
        <v-spacer></v-spacer>
      </v-card-title>
      <DeviceDetails v-if="device" class="detailsSection" :deviceId="device.id"></DeviceDetails>
    </v-card>
  </v-container>
</template>

