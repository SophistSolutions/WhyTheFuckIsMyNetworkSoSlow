
<script setup lang="ts">
import { defineComponent, defineProps, onMounted, onUnmounted, nextTick, ref, computed, ComputedRef } from 'vue';
import { useRoute } from 'vue-router'

import { IDevice } from "../models/device/IDevice";
import { useQuasar } from 'quasar';

// Components
import DeviceDetails from '../components/DeviceDetails.vue';

import { useNetStateStore } from '../stores/Net-State-store'

const $q = useQuasar()
const store = useNetStateStore()

const props = defineProps({
  selectedNetworkink: { type: String, required: false, default: null },
})

let polling: undefined | NodeJS.Timeout;

defineComponent({
  components: {
    DeviceDetails,
  },
});

const kRefreshFrequencyInSeconds_: number = 15;

onMounted(() => {
  // first time check quickly, then more gradually
  store.fetchDevice(route.params.id as string);
  store.fetchAvailableNetworks();
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    store.fetchDevice(route.params.id as string);
    store.fetchAvailableNetworks();
  }, kRefreshFrequencyInSeconds_ * 1000);
})

onUnmounted(() => {
  clearInterval(polling);
})

const route = useRoute()

let device: ComputedRef<IDevice> = computed(() => {
  let r: IDevice = store.getDevice(route.params.id as string);
  if (!r) {
    r = { id: "INVALID", attachedNetworks: {}, name: "INVALID", attachedNetworkInterfaces: [], type: [] };
  }
  return r;
}
);
</script>

<template>
  <q-page padding class=" justify-center row">
    <q-card class="pageCard col-11">
      <q-card-section class="text-subtitle2">
        Device {{ device == null ? "loading..." : '"' + device.name + '"' }}
      </q-card-section>
      <q-card-section>
        <DeviceDetails v-if="device" :deviceId="device.id" />
      </q-card-section>
    </q-card>
  </q-page>
</template>