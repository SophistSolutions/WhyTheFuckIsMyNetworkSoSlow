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
        :calculate-widths="true"
        :single-expand="true"
        :headers="headers"
        :items="devicesAsDisplayed"
        :single-select="true"
        :search="search"
        :sort-by="sortBy"
        :sort-desc="sortDesc"
        multi-sort
        disable-pagination
        :hide-default-footer="true"
        item-key="id"
      >
        <template v-slot:item.lastSeenAt="{ headers, item }">
          <td>{{ item.lastSeenAt | moment("from", "now") }}</td>
        </template>
        <template v-slot:item.type="{ headers, item }">
          <td>
            <span v-for="t in iconURLs(deviceFromID_(item.id).type)">
              <img :src="t.url" :title="t.label" height="24" width="24" />
            </span>
            <span v-for="t in nonIconTypes(deviceFromID_(item.id).type)">
              {{ t }}
            </span>
          </td>
        </template>
        <template v-slot:expanded-item="{ headers, item }">
          <td :colspan="headers.length">
            <table v-bind:key="item.id">
              <tr>
                <td>Name</td>
                <td>{{ item.name }}</td>
              </tr>
              <tr>
                <td>ID</td>
                <td>{{ item.id }}</td>
              </tr>
              <tr v-if="deviceFromID_(item.id).type">
                <td>Types</td>
                <td>{{ deviceFromID_(item.id).type.join(", ") }}</td>
              </tr>
              <tr v-if="item.icon">
                <td>Icon</td>
                <td>{{ item.icon }}</td>
              </tr>
              <tr v-if="deviceFromID_(item.id).openPorts">
                <td>Open Ports</td>
                <td>{{ deviceFromID_(item.id).openPorts.join(", ") }}</td>
              </tr>
              <tr v-if="deviceFromID_(item.id).presentationURL">
                <td>Presentation</td>
                <td>{{ deviceFromID_(item.id).presentationURL }}</td>
              </tr>
              <tr v-if="deviceFromID_(item.id).operatingSystem">
                <td>OS</td>
                <td>
                  {{ deviceFromID_(item.id).operatingSystem.fullVersionedName }}
                </td>
              </tr>
              <tr v-if="deviceFromID_(item.id).manufacturer">
                <td>Manufacturers</td>
                <td>
                  {{
                    deviceFromID_(item.id).manufacturer.fullName ||
                      deviceFromID_(item.id).manufacturer.shortName
                  }}
                </td>
              </tr>
              <tr>
                <td>Networks</td>
                <td>{{ deviceFromID_(item.id).attachedNetworks }}</td>
              </tr>
              <tr>
                <td>Last Seen</td>
                <td>{{ devicexxxFromID_(item.id).lastSeenAt | moment("from", "now") }}</td>
              </tr>
              <tr v-if="deviceFromID_(item.id).debugProps">
                <td>DEBUG INFO</td>
                <td>{{ deviceFromID_(item.id).debugProps }}</td>
              </tr>
            </table>
          </td>
        </template>
      </v-data-table>
    </v-card>
  </v-container>
</template>

<script lang="ts">
import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import { INetwork } from "@/models/network/INetwork";
import { Component, Vue, Watch } from "vue-property-decorator";

import { compareValues } from "@/CustomSort";
import { fetchNetworks } from "@/proxy/API";

@Component({
  name: "Devices",
  components: {},
})
export default class Devices extends Vue {
  private polling: undefined | number = undefined;

  private sortBy: any = [];
  private sortDesc: any = [];

  // terrible inefficient approach - maybe create map object dervied from devices array
  private deviceFromID_(id: string) {
    let result = null;
    this.devices.every((d) => {
      if (d.id === id) {
        result = d;
        return false;
      }
      return true;
    });
    return result;
  }
  private devicexxxFromID_(id: string) {
    let result = null;
    this.devicesAsDisplayed.every((d: any) => {
      if (d.id === id) {
        result = d;
        return false;
      }
      return true;
    });
    return result;
  }

  private rowClicked(e: any, row: IDevice) {
    if (!e.ctrlKey) {
      // single select unless shift key
    }
    // console.log(row);
  }

  private iconURLs(t: any) {
    const result: object[] = [];
    if (t) {
      t.forEach((ti: string) => {
        if (ti == "Router") {
          result.push({ url: "images/RouterDevice.ico", label: ti });
        } else if (ti == "Network-Infrastructure") {
          result.push({ url: "images/network-infrastructure.ico", label: ti });
        } else if (ti == "Personal-Computer") {
          result.push({ url: "images/PC-Device.png", label: ti });
        }
      });
    }
    return result;
  }
  private nonIconTypes(t: any) {
    const result: string[] = [];
    if (t) {
      t.forEach((ti: string) => {
        if (ti == "Router") {
        } else if (ti == "Network-Infrastructure") {
        } else if (ti == "Personal-Computer") {
        } else {
          result.push(ti);
        }
      });
    }
    return result;
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
        value: "manufacturer",
      },
      {
        text: "OS",
        value: "operatingSystem",
      },
      {
        text: "Network",
        value: "networks",
      },
      {
        text: "Local Address",
        value: "localAddrresses",
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
    setTimeout(() => {
      this.fetchDevices();
    }, 5 * 1000);
    this.polling = setInterval(() => {
      this.fetchDevices();
      this.fetchAvailableNetworks();
    }, 15 * 1000);
  }

  private get devices(): IDevice[] {
    return this.$store.getters.getDevices;
  }

  @Watch("devices()")
  private get devicesAsDisplayed(): object[] {
    const result: object[] = [];
    this.devices.forEach((i) => {
      result.push({
        id: i.id,
        name: i.name,
        type: i.type == null ? null : i.type.join(", "),
        lastSeenAt: i.lastSeenAt,
        manufacturer:
          i.manufacturer == null ? "" : i.manufacturer.fullName || i.manufacturer.shortName,
        os: i.operatingSystem == null ? null : i.operatingSystem.fullVersionedName,
        networks: this.formatNetworks_(i.attachedNetworks),
        localAddrresses: this.formatNetworkAddresses_(i.attachedNetworks),
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

// .selectedDevicesSection {
//   margin-top: 10px;
//   margin-left: 30px;
// }

// .deviceList td {
//   white-space: nowrap;
//   overflow: hidden;
//   text-overflow: ellipsis;
// }

// .deviceRow {
//   width: 100vw;
//   max-width: 100vw;

//   > td {
//     // white-space: nowrap;
//     overflow: hidden;
//     text-overflow: ellipsis;
//   }

//   &:hover {
//     background-color: yellow !important;
//   }
// }

// .selectedRow {
//   background-color: lightyellow;

//   &:hover {
//     background-color: yellow !important;
//   }
// }
</style>
