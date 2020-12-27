<template>
  <v-container class="devicesPage">
    <v-card class="deviceListCard">
      <v-card-title>
        Devices {{ $route.params.id }}
        <v-spacer></v-spacer>
      </v-card-title>
      <DeviceDetails :device="device" :networks="networks"></DeviceDetails>
    </v-card>
  </v-container>
</template>

<script lang="ts">
import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import { INetwork } from "@/models/network/INetwork";

import { Component, Vue, Watch } from "vue-property-decorator";

@Component({
  name: "Device",
  components: {
    DeviceDetails: () => import("@/components/DeviceDetails.vue"),
  },
})
export default class Device extends Vue {
  private polling: undefined | number = undefined;

  private fetchAvailableNetworks() {
    this.$store.dispatch("fetchAvailableNetworks");
  }
  private get networks(): INetwork[] {
    return this.$store.getters.getAvailableNetworks;
  }

  private created() {
    this.fetchDevices();
    this.fetchAvailableNetworks();
    this.pollData();
  }

  private beforeDestroy() {
    clearInterval(this.polling);
  }

  private fetchDevices() {
    this.$store.dispatch("fetchDevices", null);
  }

  private pollData() {
    // first time check quickly, then more gradually
    this.fetchDevices();
    this.fetchAvailableNetworks();
    this.polling = setInterval(() => {
      this.fetchDevices();
      this.fetchAvailableNetworks();
    }, 15 * 1000);
  }

  private get device(): IDevice {
    let r = null;
    this.devices.forEach((i) => {
      // console.log("i=", i);
      if (i.id === this.$route.params.id) {
        r = i;
      }
    });
    if (r === null) {
      r = { id: "INVALID", attachedNetworks: {}, name: "INVALID", type: "INVALID" as any };
    }
    return r;
  }

  private get devices(): IDevice[] {
    return this.$store.getters.getDevices;
  }
}
</script>

<style lang="scss"></style>
