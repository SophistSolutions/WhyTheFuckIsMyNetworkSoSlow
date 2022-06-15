
<script setup lang="ts">
import { defineComponent, defineProps, onMounted, onUnmounted, nextTick, ref, computed, ComputedRef } from 'vue';
import { useRoute } from 'vue-router'

import { IDevice } from "../models/device/IDevice";
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

const kRefreshFrequencyInSeconds_: number = 15;

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
  }, kRefreshFrequencyInSeconds_ * 1000);
})

onUnmounted(() => {
  clearInterval(polling);
})

const route = useRoute()

let device: ComputedRef<IDevice> = computed(() => {
  let r: IDevice = store.getDevice(route.params.id as string);
  if (!r) {
    r = { id: "INVALID", attachedNetworks: {}, name: "INVALID", attachedNetworkInterfaces: [], type: null };
  }
  return r;
}
);
</script>

<template>
  <q-page class="col q-gutter-md">
    <q-card>
      <q-card-section class="text-subtitle2" style="margin: 0 0 0 0">
        Device {{ device == null ? "loading..." : '"' + device.name + '"' }}
      </q-card-section>
      <q-card-section style="margin-top: 0">
        <DeviceDetails v-if="device" class="detailsSection" :deviceId="device.id" />
      </q-card-section>
    </q-card>
  </q-page>
</template>

<style lang="scss">
.detailsSection {
  margin-left: 2em;
}
</style>
