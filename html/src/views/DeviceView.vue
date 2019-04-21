<template>
  <div class="devices">
    <div class="deviceList" v-for="device in sortedDevices" :key="device.id">
      <Device name="Device" :device="device"></Device>
    </div>
  </div>
</template>

<script lang="ts">
import { IDevice } from "@/models/device/IDevice";
import { Component, Vue } from "vue-property-decorator";

import { compareValues } from "@/CustomSort";
import { fetchNetworks } from "@/proxy/API";

@Component({
  name: "Devices",
  components: {
    Device: () => import("@/components/Device.vue")
  }
})
export default class Devices extends Vue {
  private polling: undefined | number = undefined;

  private sortField: string = "name";
  private sortOrder: string = "asc";

  private get sortedDevices(): IDevice[] {
    let deviceArray: IDevice[] = this.devices;
    return deviceArray.sort(compareValues(this.sortField, this.sortOrder));
  }

  private created() {
    this.fetchDevices();
    this.pollData();
  }

  private beforeDestroy() {
    clearInterval(this.polling);
  }

  private fetchDevices() {
    this.$store.dispatch("fetchDevices");
  }

  private pollData() {
    this.polling = setInterval(() => {
      this.fetchDevices();
    }, 10000);
  }

  private get devices(): IDevice[] {
    const devices: IDevice[] = this.$store.getters.getDevices;
    // shallow clone
    const ret = [...devices];
    return ret.sort();

    // TODO move sort to store

    //return (this.$store.getters.getDevices) ? JSON.parse(JSON.stringify(devices)) : [];
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