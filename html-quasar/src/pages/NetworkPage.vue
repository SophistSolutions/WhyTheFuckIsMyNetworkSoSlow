<script setup lang="ts">
import { defineComponent, defineProps, onMounted, onUnmounted, nextTick, ref, computed, ComputedRef } from 'vue';
import { useRoute } from 'vue-router'
import { useQuasar } from 'quasar';

import { INetwork } from "../models/network/INetwork";
import { GetNetworkName } from "../models/network/Utils";

// Components
import NetworkDetails from '../components/NetworkDetails.vue';

import { useNetStateStore } from '../stores/Net-State-store'

const $q = useQuasar()
const store = useNetStateStore()

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
  <q-page>
    <q-card>
      <q-card-section class="text-subtitle2" style="margin: 0 0 0 0">
        Network {{ network == null ? "loading..." : '"' + GetNetworkName(network) + '"' }}
      </q-card-section>
      <q-card-section style="margin-top: 0">
        <NetworkDetails class="detailsSection" :networkId="network.id" v-if="network" />
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
