
<script setup lang="ts">
import { watch, defineProps, onMounted, onUnmounted, computed, ComputedRef } from 'vue';
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

const emit = defineEmits(['update:breadcrumbs'])


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

let device: ComputedRef<IDevice | null> = computed(() => {
  return store.getDevice(route.params.id as string);
});


watch(
  () => device.value,
  async device => {
    // @todo - check network.names[0] - LENGTH - handle emopty case
    // @todo CODE sharing with predefined routes
    if (device) {
      if (device.aggregatedBy) {
        emit('update:breadcrumbs',  [
          { text: 'Home', href: '/#/' },
          { text: 'Devices', href: '/#/devices' },
          // @todo wrong name for parent network name possibly - must fetch aggregated by and use its name - but not worth the trouble now since almost certainly the same
          { text: device.names[0].name, href: '/#/device/' + device.aggregatedBy,  },
          // @todo replace this name with the 'pretty seen' string we use 
          { text: device.names[0].name, disabled: true },
        ])
      }
      else {
        emit('update:breadcrumbs',  [
          { text: 'Home', href: '/#/' },
          { text: 'Devices', href: '/#/devices' },
          { text: device.names[0].name, disabled: true },
        ])
      }
    }
  }
)
</script>

<template>
  <q-page padding class=" justify-center row">
    <q-card class="pageCard col-11">
      <q-card-section class="text-subtitle2">
        Device Details for {{ device == null ? "loading..." : '"' + device.name + '"' }}
      </q-card-section>
      <q-card-section>
        <DeviceDetails v-if="device" :deviceId="device.id" :showExtraDetails="true" />
      </q-card-section>
    </q-card>
  </q-page>
</template>