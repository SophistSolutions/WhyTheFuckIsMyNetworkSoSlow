<script setup lang="ts">
import { onMounted, onUnmounted, computed, ComputedRef } from 'vue';
import { useRoute } from 'vue-router';
import { useQuasar } from 'quasar';
import { watch } from 'vue';
import { INetwork } from '../models/network/INetwork';
import { GetNetworkName, FormatIDateTimeRange } from '../models/network/Utils';

import NetworkDetails from '../components/NetworkDetails.vue';

import { useNetStateStore } from '../stores/Net-State-store';

const $q = useQuasar();
const store = useNetStateStore();

let polling: undefined | NodeJS.Timeout;
const route = useRoute();

const emit = defineEmits(['update:breadcrumbs']);

onMounted(() => {
  // first time check quickly, then more gradually
  store.fetchNetworks([route.params.id as string]);
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    store.fetchNetworks([route.params.id as string]);
  }, 15 * 1000);
});

onUnmounted(() => {
  clearInterval(polling);
});

let network: ComputedRef<INetwork> = computed(() => {
  return store.getNetwork(route.params.id as string);
});

watch(
  () => network.value,
  async (network) => {
    // @todo - check network.names[0] - LENGTH - handle emopty case
    // @todo CODE sharing with predefined routes
    if (network) {
      if (network.aggregatedBy) {
        emit('update:breadcrumbs', [
          { text: 'Home', href: '/#/' },
          { text: 'Networks', href: '/#/networks' },
          // @todo wrong name for parent network name possibly - must fetch aggregated by and use its name - but not worth the trouble now since almost certainly the same
          {
            text: network.names[0].name,
            href: '/#/network/' + network.aggregatedBy,
          },
          {
            text: FormatIDateTimeRange(network.seen, true) ?? '?',
            disabled: true,
          },
        ]);
      } else {
        emit('update:breadcrumbs', [
          { text: 'Home', href: '/#/' },
          { text: 'Networks', href: '/#/networks' },
          { text: network.names[0].name, disabled: true },
        ]);
      }
    }
  }
  ,{immediate:true}
);
</script>

<template>
  <q-page padding class="justify-center row">
    <q-card class="pageCard col-11">
      <q-card-section class="text-subtitle2" style="padding-bottom: 0">
        Network
        {{
          network == null ? 'loading...' : '"' + GetNetworkName(network) + '"'
        }}
        <span class="snapshot" v-if="network?.aggregatedBy">{snapshot}</span>
      </q-card-section>
      <q-card-section style="margin-top: 0">
        <NetworkDetails
          :network="network"
          v-if="network"
          :showExtraDetails="true"
          :allowEdit="true"
        />
      </q-card-section>
    </q-card>
  </q-page>
</template>
