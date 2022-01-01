<template>
  <v-container>
    <app-bar />
    <v-card class="deviceListCard">
      <v-card-title>
        Device {{ device == null ? "loading..." : '"' + device.name + '"' }}
        <v-spacer></v-spacer>
      </v-card-title>
      <DeviceDetails v-if="device" class="detailsSection" :deviceId="device.id"></DeviceDetails>
    </v-card>
  </v-container>
</template>

<script lang="ts">
import { IDevice } from "@/models/device/IDevice";
import { INetwork } from "@/models/network/INetwork";

import { Options, Vue } from 'vue-class-component'

@Options({
  name: "Device",
  components: {
    AppBar: () => import("@/components/AppBar.vue"),
    DeviceDetails: () => import("@/components/DeviceDetails.vue"),
  },
})
export default class Device extends Vue {
  private polling: undefined | number = undefined;

  public created() {
    this.pollData();
  }

  public beforeDestroy() {
    clearInterval(this.polling);
  }

  private pollData() {
    // first time check quickly, then more gradually
    this.$store.dispatch("fetchDevice", this.$route.params.id);
    if (this.polling) {
      clearInterval(this.polling);
    }
    this.polling = setInterval(() => {
      this.$store.dispatch("fetchDevice", this.$route.params.id);
    }, 15 * 1000);
  }

  private get device(): IDevice {
    return this.$store.getters.getDevice(this.$route.params.id);
  }
}
</script>

<style lang="scss">
.detailsSection {
  margin-left: 2em;
}
</style>
