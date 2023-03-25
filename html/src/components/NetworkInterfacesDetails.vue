<script setup lang="ts">
import { PropType } from 'vue';

import NetworkInterfaceDetails from '../components/NetworkInterfaceDetails.vue';

import { useNetStateStore } from '../stores/Net-State-store';

const props = defineProps({
  networkInterfaceIds: {
    type: Array as PropType<Array<string>>,
    required: true,
  },
  showInactiveInterfaces: { type: Boolean, required: false, default: false },
});
</script>

<template>
  <div v-if="true" class="q-pa-sm">
    <div
      class="row"
      v-for="interfaceId in props.networkInterfaceIds"
      v-bind:key="interfaceId"
    >
      <div class="col">
        <NetworkInterfaceDetails
          :networkInterfaceId="interfaceId"
          :showInactiveInterfaces="props.showInactiveInterfaces"
        />
      </div>
    </div>
  </div>
</template>

<style scoped lang="scss"></style>
