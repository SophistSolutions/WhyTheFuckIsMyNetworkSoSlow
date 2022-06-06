
<script setup lang="ts">
import { defineComponent, defineProps, onMounted, onUnmounted, nextTick, ref, computed, ComputedRef } from 'vue';
import { useRoute } from 'vue-router'
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
import DeviceDetails from '../components/DeviceDetails.vue';

import { useWTFStore } from '../stores/WTF-store'

const $q = useQuasar()
const store = useWTFStore()

const props = defineProps({
  selectedNetworkink: { type: String, required: false, default: null },
})

let polling: undefined | NodeJS.Timeout;

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

let device: ComputedRef<IDevice> = computed(() => {
    let r : IDevice = store.getDevice(route.params.id as string);
    if (!r) {
      r = { id: "INVALID", attachedNetworks: {}, name: "INVALID", attachedNetworkInterfaces:[], type: null };
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
  <q-page class="col q-pa-md q-gutter-md">
    <q-card class="deviceListCard">
      <v-card-title>
        Device {{ device == null ? "loading..." : '"' + device.name + '"' }}
      </v-card-title>
      <DeviceDetails v-if="device" class="detailsSection" :deviceId="device.id"></DeviceDetails>
    </q-card>
  </q-page>
</template>

