<template>
  <v-container>
    <app-bar />
    <v-card>
      <v-card-title>
        Network "{{ GetNetworkName(network) }}"
        <v-spacer></v-spacer>
      </v-card-title>
      <NetworkDetails
        class="detailsSection"
        :network="network"
        :devices="devices"
        :networkInterfaces="networkInterfaces"
      />
    </v-card>
  </v-container>
</template>

<script lang="ts">
import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import { INetwork } from "@/models/network/INetwork";
import { INetworkInterface } from "@/models/network/INetworkInterface";
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

  private get networks(): INetwork[] {
    return this.$store.getters.getAvailableNetworks;
  }

  private get networkInterfaces(): INetworkInterface[] {
    return this.$store.getters.getNetworkInterfaces;
  }

  private created() {
    this.pollData();
  }

  private beforeDestroy() {
    clearInterval(this.polling);
  }

  private pollData() {
    // first time check quickly, then more gradually
    this.$store.dispatch("fetchDevices");
    this.$store.dispatch("fetchAvailableNetworks");
    this.$store.dispatch("fetchNetworkInterfaces");
    this.polling = setInterval(() => {
      this.$store.dispatch("fetchDevices");
      this.$store.dispatch("fetchAvailableNetworks");
      this.$store.dispatch("fetchNetworkInterfaces");
    }, 15 * 1000);
  }

  private get network(): INetwork {
    let r: INetwork | null = null;
    this.networks.forEach((i) => {
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
