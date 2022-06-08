<script setup lang="ts">
import { defineComponent, defineProps, onMounted, onUnmounted, nextTick, ref, computed, ComputedRef } from 'vue';
import { useRoute } from 'vue-router'
import { useQuasar } from 'quasar';

import { INetwork } from "../models/network/INetwork";
import { GetNetworkName } from "../models/network/Utils";

// Components
import NetworkDetails from '../components/NetworkDetails.vue';

import { useWTFStore } from '../stores/WTF-store'

const $q = useQuasar()
const store = useWTFStore()

const props = defineProps({
  selectedNetworkink: { type: String, required: false, default: null },
})

let polling: undefined | NodeJS.Timeout;
const route = useRoute()

defineComponent({
  components: {
    NetworkDetails,
  },
});

onMounted(() => {
  // first time check quickly, then more gradually
  store.fetchNetworks([route.params.id as string]);
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    store.fetchNetworks([route.params.id as string]);
  }, 15 * 1000);
})

onUnmounted(() => {
  clearInterval(polling);
})

let network: ComputedRef<INetwork> = computed(() => {
  return store.getNetwork(route.params.id as string);
});
</script>

<template>
  <q-page class="col q-pa-md q-gutter-md">
    <q-card class="deviceListCard">
      <div class="text-subtitle2 absolute-top text-center">
        Network {{ network == null ? "loading..." : '"' + GetNetworkName(network) + '"' }}
      </div>
      <NetworkDetails class="detailsSection" :networkId="network.id" v-if="network" />
    </q-card>
  </q-page>
</template>


<style lang="scss">
.detailsSection {
  margin-left: 2em;
}
</style>
