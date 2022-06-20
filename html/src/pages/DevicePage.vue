
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
  <q-page>
    <q-card>
      <q-card-section class="text-subtitle2">
        Device {{ device == null ? "loading..." : '"' + device.name + '"' }}
      </q-card-section>
      <q-card-section class="detailsSection">
        <DeviceDetails v-if="device" :deviceId="device.id" />
      </q-card-section>
    </q-card>
  </q-page>
</template>

<style lang="scss" scoped>
.detailsSection {
  margin: 0em 2em;
  box-shadow: 4px 4px 8px 4px rgba(0, 0, 0, 0.2);
  transition: 0.3s;
}

.detailsSection:hover {
  box-shadow: 4px 4px 8px 4px rgba(0, 0, 0, 0.2);
}
</style>
