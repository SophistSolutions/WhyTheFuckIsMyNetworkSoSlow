<script setup lang="ts">
import { defineProps, onMounted, defineEmits, onUnmounted, computed, ComputedRef } from 'vue';
import { useRoute } from 'vue-router'
import { useQuasar } from 'quasar';
import { watch } from 'vue'
import { INetworkInterface } from "../models/network/INetworkInterface";

import NetworkInterfaceDetails from '../components/NetworkInterfaceDetails.vue';

import { useNetStateStore } from '../stores/Net-State-store'

const $q = useQuasar()
const store = useNetStateStore()

const props = defineProps({
  selectedNetworkink: { type: String, required: false, default: null },
})

let polling: undefined | NodeJS.Timeout;
const route = useRoute()

const emit = defineEmits(['update:breadcrumbs'])

onMounted(() => {
  // first time check quickly, then more gradually
  store.fetchNetworkInterfaces([route.params.id as string]);
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    store.fetchNetworkInterfaces([route.params.id as string]);
  }, 15 * 1000);
})

onUnmounted(() => {
  clearInterval(polling);
})

let networkInterface: ComputedRef<INetworkInterface> = computed(() => {
  return store.getNetworkInterface(route.params.id as string);
});

watch(
  () => networkInterface.value,
  async networkInterface => {
    // @todo - check network.names[0] - LENGTH - handle emopty case
    // @todo CODE sharing with predefined routes
    if (networkInterface) {
      if (networkInterface.aggregatedBy) {
        emit('update:breadcrumbs',  [
          { text: 'Home', href: '/#/' },
          // @todo wrong name for parent network name possibly - must fetch aggregated by and use its name - but not worth the trouble now since almost certainly the same
          { text: networkInterface.friendlyName, href: '/#/network-interface/' + networkInterface.aggregatedBy,  },
          // @todo replace this name with the 'pretty seen' string we use 
          { text: networkInterface.friendlyName, disabled: true },
        ])
      }
      else {
        emit('update:breadcrumbs',  [
          { text: 'Home', href: '/#/' },
          { text: networkInterface.friendlyName, disabled: true },
        ])
      }
    }
  }
)
</script>

<template>
  <q-page padding class=" justify-center row">
    <q-card class="pageCard col-11">
      <q-card-section class="text-subtitle2" style="margin: 0 0 0 0">
        NetworkInterface {{ networkInterface == null ? "loading..." : '"' + networkInterface.friendlyName + '"' }}
      </q-card-section>
      <q-card-section style="margin-top: 0">
        <NetworkInterfaceDetails :networkInterface="networkInterface" v-if="networkInterface" :showExtraDetails="true" />
      </q-card-section>
    </q-card>
  </q-page>
</template>
