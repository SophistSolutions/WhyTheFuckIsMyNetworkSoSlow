<template>
  <v-container class="devicesPage">
    <v-card class="deviceListCard">
      <v-card-title>
        Devices
        <v-spacer></v-spacer>
      </v-card-title>
      <v-data-table
        fixed-header
        class="deviceList elevation-1"
        dense
        show-expand
        :expanded.sync="expanded"
        :single-expand="true"
        :headers="headers"
        :items="deviceRows"
        :single-select="true"
        :search="search"
        :sort-by="sortBy"
        :sort-desc="sortDesc"
        multi-sort
        disable-pagination
        :hide-default-footer="true"
        item-key="id"
        @click:row="rowClicked"
      >
        <template v-slot:item.lastSeenAt="{ headers, item }">
          <td>
            <span v-if="item.lastSeenAt">{{ item.lastSeenAt | moment("from", "now") }}</span>
          </td>
        </template>
        <template v-slot:item.type="{ headers, item }">
          <td>
            <span v-for="t in computeDeviceTypeIconURLs_(item.type)">
              <img v-if="t.url" :src="t.url" :title="t.label" height="24" width="24" />
              <span v-if="!t.url">
                {{ t.label }}
              </span>
            </span>
          </td>
        </template>
        <template v-slot:item.operatingSystem="{ headers, item }">
          <td>
            <span v-for="t in computeOSIconURLList_(item.operatingSystem)">
              <img v-if="t.url" :src="t.url" :title="t.label" height="24" width="24" />
              <span v-if="!t.url">
                {{ t.label }}
              </span>
            </span>
          </td>
        </template>
        <template v-slot:expanded-item="{ headers, item }">
          <td :colspan="headers.length">
            <DeviceDetails :device="item" :networks="networks"></DeviceDetails>
          </td>
        </template>
      </v-data-table>
    </v-card>
  </v-container>
</template>

<script lang="ts">
import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import { ComputeDeviceTypeIconURLs, ComputeOSIconURLList } from "@/models/device/Utils";
import { INetwork } from "@/models/network/INetwork";
import { OperatingSystem } from "@/models/OperatingSystem";

import { Component, Vue, Watch } from "vue-property-decorator";

import { fetchNetworks } from "@/proxy/API";

@Component({
  name: "Devices",
  components: {
    DeviceDetails: () => import("@/components/DeviceDetails.vue"),
  },
})
export default class Devices extends Vue {
  private polling: undefined | number = undefined;

  private sortBy: any = [];
  private sortDesc: any = [];
  private expanded: any[] = [];

  private rowClicked(row: any) {
    // @todo Try this again with vue3 - https://github.com/vuetifyjs/vuetify/issues/9720
    // if (!e.ctrlKey) {
    //   // single select unless shift key
    //
    const index = this.expanded.indexOf(row);
    this.expanded = [];
    if (index === -1) {
      this.expanded.push(row);
    }
  }

  // for now use private method since cannot access global functions from template??? Ask John?
  private computeDeviceTypeIconURLs_(t: string[] | null) {
    return ComputeDeviceTypeIconURLs(t);
  }
  private computeOSIconURLList_(t: OperatingSystem | null) {
    return ComputeOSIconURLList(t);
  }

  private fetchAvailableNetworks() {
    this.$store.dispatch("fetchAvailableNetworks");
  }
  private get networks(): INetwork[] {
    return this.$store.getters.getAvailableNetworks;
  }

  private formatNetworks_(attachedNetworks: { [key: string]: INetworkAttachmentInfo }): string {
    let addresses: string[] = [];
    Object.entries(attachedNetworks).forEach((element) => {
      let netID = element[0];
      this.networks.forEach((network: INetwork) => {
        if (network.id === netID) {
          if (network.networkAddresses.length >= 1) {
            netID = network.networkAddresses[0];
          }
        }
      });
      addresses.push(netID);
    });
    addresses = addresses.filter((value, index, self) => self.indexOf(value) === index);
    return addresses.join(", ");
  }

  private formatNetworkAddresses_(attachedNetworks: {
    [key: string]: INetworkAttachmentInfo;
  }): string {
    let addresses: string[] = [];
    Object.entries(attachedNetworks).forEach((element) => {
      // console.log(element);
      element[1].networkAddresses.forEach((e: string) => addresses.push(e));
    });
    addresses = addresses.filter((value, index, self) => self.indexOf(value) === index);
    return addresses.join(", ");
  }

  private get search(): string {
    return this.$store.getters.getSearchString;
  }

  private get headers(): object[] {
    return [
      {
        text: "Name",
        align: "start",
        value: "name",
      },
      {
        text: "Type",
        value: "type",
      },
      {
        text: "Last Seen",
        value: "lastSeenAt",
      },
      {
        text: "Manufacturer",
        value: "manufacturerSummary",
      },
      {
        text: "OS",
        value: "operatingSystem",
      },
      {
        text: "Network",
        value: "networksSummary",
      },
      {
        text: "Local Address",
        value: "localAddresses",
      },
      {
        text: "Details",
        value: "data-table-expand",
      },
    ];
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

  private get devices(): IDevice[] {
    return this.$store.getters.getDevices;
  }

  private get deviceRows(): object[] {
    const result: object[] = [];
    this.devices.forEach((i) => {
      result.push({
        ...i,
        ...{
          networksSummary: this.formatNetworks_(i.attachedNetworks),
          localAddresses: this.formatNetworkAddresses_(i.attachedNetworks),
          manufacturerSummary:
            i.manufacturer == null ? "" : i.manufacturer.fullName || i.manufacturer.shortName,
        },
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
}
</script>

<style lang="scss">
.devicesPage {
}

.deviceListCard {
  margin-top: 10px;
  margin-left: 10px;
}

.deviceList {
  margin-top: 10px;
}
</style>
