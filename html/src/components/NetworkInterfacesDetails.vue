<script setup lang="ts">
import { PropType, defineProps, defineComponent, onMounted, onUnmounted, ref, computed, ComputedRef } from 'vue';

import { INetworkInterface } from "../models/network/INetworkInterface";

// Components
import NetworkInterfaceDetails from '../components/NetworkInterfaceDetails.vue';

import { useNetStateStore } from '../stores/Net-State-store'
import { INetwork } from 'src/models/network/INetwork';

const store = useNetStateStore()

const props = defineProps({
  networkInterfaceIds: { type: Array as PropType<Array<string>>, required: true },
})

defineComponent({
  components: {
  },
});
</script>

<template>
  <div v-if="true" class="q-pa-sm">
    <div class="row" v-for="interfaceId in props.networkInterfaceIds" v-bind:key="interfaceId">
      <div class="col">
        <NetworkInterfaceDetails :networkInterfaceId="interfaceId" />
      </div>
    </div>
  </div>
</template>

<style scoped lang="scss">
</style>