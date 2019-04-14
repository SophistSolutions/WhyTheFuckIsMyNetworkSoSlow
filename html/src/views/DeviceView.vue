<template>
  <div class="devices">
  <div class="deviceList" v-for="device in devices" :key="device.id">
    <Device name=Device  :device=device></Device>
  </div>
  </div>
</template>

<script lang="ts">
import { IDevice } from "@/models/device/IDevice";
import { Component, Vue } from "vue-property-decorator";

import { fetchNetworks } from "@/proxy/API";

@Component({
    name : "Devices",
    components : {
      Device: () => import("@/components/Device.vue"),
    },
})
export default class Devices extends Vue {

    private polling : undefined | number = undefined;

    private created() {
      this.fetchDevices();
      this.pollData();
    }

    private beforeDestroy() {
      clearInterval(this.polling);
    }

    private fetchDevices() {
      this.$store.dispatch('fetchDevices');
    }

    private pollData() {
      this.polling = setInterval(() => {
        this.fetchDevices();
      }, 10000);
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