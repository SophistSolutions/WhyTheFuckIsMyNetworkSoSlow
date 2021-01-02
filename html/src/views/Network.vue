<template>
  <v-container>
    <app-bar />
    <v-card>
      <v-card-title>
        Network "{{ GetNetworkName(network) }}" ({{ $route.params.id }})
        <v-spacer></v-spacer>
      </v-card-title>
      <NetworkDetails class="detailsSection" :network="network" :devices="devices" />
    </v-card>
  </v-container>
</template>

<script lang="ts">
import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import { INetwork } from "@/models/network/INetwork";
import { GetNetworkName } from "@/models/network/Utils";

import { Component, Vue, Watch } from "vue-property-decorator";

@Component({
  name: "Network",
  components: {
    AppBar: () => import("@/components/AppBar.vue"),
    NetworkDetails: () => import("@/components/NetworkDetails.vue"),
  },
})
export default class Network extends Vue {
  private polling: undefined | number = undefined;

  private GetNetworkName = GetNetworkName;

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

  private get network(): INetwork {
    let r: INetwork | null = null;
    this.networks.forEach((i) => {
      // console.log("i=", i);
      if (i.id === this.$route.params.id) {
        r = i;
      }
    });
    if (r === null) {
      r = {
        id: "INVALID",
        DNSServers: [],
        attachedInterfaces: [],
        externalAddresses: [],
        gateways: [],
        geographicLocation: {
          city: "",
          coordinates: {} as any,
          countryCode: "",
          postalCode: "",
          regionCode: "",
        },
        internetServiceProvider: { name: "INVALID" },
        networkAddresses: [],
      };
    }
    return r;
  }

  private get devices(): IDevice[] {
    return this.$store.getters.getDevices;
  }
}
</script>

<style lang="scss">
.detailsSection {
  margin-left: 2em;
}
</style>
