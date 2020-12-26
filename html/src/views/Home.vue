<template>
  <v-container>
    <v-row class="text-center">
      <v-col class="mb-4">
        <h1 class="display-2 font-weight-bold mb-3">
          Why The Fuck is My Network So Slow?
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
              : {{ getDevicesInNetwork(network).length }}
              <router-link to="/devices">devices</router-link>, operating normally
            </div>
          </li>
        </ul>
      </v-col>
    </v-row>
  </v-container>
</template>

<script lang="ts">
import Vue from "vue";
import Component from "vue-class-component";
import { INetwork } from "@/models/network/INetwork";
import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import { Component, Vue, Watch } from "vue-property-decorator";

@Component({
  name: "Home",
  components: {},
})
export default class Home extends Vue {
  private polling: undefined | number = undefined;

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
    let allNetworks: INetwork[] = this.$store.getters.getAvailableNetworks;
    let result: INetwork[] = [];
    allNetworks.forEach((i) => {
      if (i.internetServiceProvider != null || i.geographicLocation != null) {
        result.push(i);
      }
    });
    return result;
  }

  private getDevicesInNetwork(nw: INetwork): string[] {
    const ids: string[] = [];
    this.devices.forEach((i: IDevice) => {
      let hasThisNetwork = false;
      Object.entries(i.attachedNetworks).forEach((element) => {
        if (element[0] === nw.id) {
          hasThisNetwork = true;
        }
      });
      if (hasThisNetwork) {
        ids.push(i.id);
      }
    });
    return ids;
  }

  @Watch("networks()")
  private get networksAsDisplayed(): object[] {
    const result: object[] = [];
    this.networks.forEach((i) => {
      const location: string | null =
        i.geographicLocation == null
          ? null
          : i.geographicLocation.city + ", " + i.geographicLocation.regionCode;
      result.push({
        id: i.id,
        name: i.networkAddresses.join(", "),
        active: "true",
        internetInfo:
          (i.gateways == null ? "" : i.gateways.join(", ")) +
          (i.internetServiceProvider == null ? " " : " (" + i.internetServiceProvider.name + ")"),
        devices: this.getDevicesInNetwork(i).length,
        status: "healthy",
        location,
        // name: i.name,
        // type: i.type == null ? null : i.type.join(", "),
        // manufacturer: i.manufacturer == null ? "?" : i.manufacturer.fullName,
        // os: i.operatingSystem == null ? null : i.operatingSystem.fullVersionedName,
        // networks: this.formatNetworks_(i.attachedNetworks),
        // localAddrresses: this.formatNetworkAddresses_(i.attachedNetworks),
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

  private getDevicesInNetwork(nw: INetwork): string[] {
    const ids: string[] = [];
    this.devices.forEach((i: IDevice) => {
      let hasThisNetwork = false;
      Object.entries(i.attachedNetworks).forEach((element) => {
        if (element[0] === nw.id) {
          hasThisNetwork = true;
        }
      });
      if (hasThisNetwork) {
        ids.push(i.id);
      }
    });
    return ids;
  }
}
</script>
