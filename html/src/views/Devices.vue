<template>
  <div class="devices">
  <div class="deviceList" v-for="device in devices" :key="device.id">
    <Device name=Device  :device=device></Device>
  </div>
  </div>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { IDevice } from '@/models/device/IDevice';

import { fetchNetworks } from '@/proxy/API';

@Component({
    name : 'Devices',
    components : {
      Device: () => import('@/views/Device.vue'),
    },
})
export default class Devices extends Vue {
  // TODO fix so networks doesnt call fetch everytime restarted
    // Trying to replace this with calls to the store instead of directly calling API
    private created() {
      this.$store.dispatch('fetchDevices');
    }

    private get devices(): IDevice[] {
      return this.$store.getters.getDevices;
    }
}
</script>

<style lang="scss">
.devices {
  position: relative;
  max-width: 70%;
  margin: auto;
  margin-top: 20px;
}
.deviceList {
  margin-top: 10px;
}
</style>