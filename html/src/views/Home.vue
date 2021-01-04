<template>
  <v-container>
    <app-bar />
    <v-row class="text-center">
      <v-col class="mb-4">
        <h1 class="display-2 font-weight-bold mb-3">
          Why the Fuck is My Network So Slow?
        </h1>
      </v-col>
    </v-row>
    <v-row>
      <v-col class="mb-4">
        <router-link to="/networks">Networks</router-link> (active + favorites)
        <ul>
          <li v-for="network in networksAsDisplayed" :key="network.id">
            {{ network.name }}
            <div>
              : {{ GetDeviceIDsInNetwork(network, devices).length }}
              <router-link to="/devices">devices</router-link>, operating normally
            </div>
          </li>
        </ul>
      </v-col>
    </v-row>
  </v-container>
</template>

<script lang="ts">
import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import { INetwork } from "@/models/network/INetwork";
import { FormatLocation, GetDeviceIDsInNetwork, GetNetworkName } from "@/models/network/Utils";
import Vue from "vue";
import { Component, Watch } from "vue-property-decorator";

@Component({
  name: "Home",
  components: {
    AppBar: () => import("@/components/AppBar.vue"),
  },
})
export default class Home extends Vue {
  private polling: undefined | number = undefined;

  private GetDeviceIDsInNetwork = GetDeviceIDsInNetwork;

  private created() {
    this.$store.dispatch("fetchDevices", null);
    this.$store.dispatch("fetchAvailableNetworks");
    this.pollData();
  }

  private beforeDestroy() {
    clearInterval(this.polling);
  }

  private pollData() {
    this.polling = setInterval(() => {
      this.$store.dispatch("fetchDevices", null);
      this.$store.dispatch("fetchAvailableNetworks");
    }, 15 * 1000);
  }

  private get networks(): INetwork[] {
    const allNetworks: INetwork[] = this.$store.getters.getAvailableNetworks;
    const result: INetwork[] = [];
    allNetworks.forEach((i) => {
      if (i.internetServiceProvider != null || i.geographicLocation != null) {
        result.push(i);
      }
    });
    return result;
  }

  private get networksAsDisplayed(): object[] {
    const result: object[] = [];
    this.networks.forEach((i) => {
      result.push({
        id: i.id,
        name: GetNetworkName(i),
        active: "true",
        internetInfo:
          (i.gateways == null ? "" : i.gateways.join(", ")) +
          (i.internetServiceProvider == null ? " " : " (" + i.internetServiceProvider.name + ")"),
        devices: GetDeviceIDsInNetwork(i, this.devices).length,
        status: "healthy",
        location: FormatLocation(i.geographicLocation),
      });
    });
    result.sort((a: any, b: any) => {
      if (a.id < b.id) {
        return -1;
      }
      if (a.id > b.id) {
        return 1;
      }
      return 0;
    });
    return result;
  }

  private get devices(): IDevice[] {
    return this.$store.getters.getDevices;
  }
}
</script>
