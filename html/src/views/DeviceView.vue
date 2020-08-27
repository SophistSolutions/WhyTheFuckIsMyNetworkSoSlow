<template>
  <v-container class="devicesPage" v-click-outside="clearSelectionIfOutsideDevicesContainer">
    <v-card class="deviceListCard">
      <v-card-title>
        Devices
        <v-spacer></v-spacer>
      </v-card-title>
      <v-data-table
        height="39vh"
        fixed-header
        class="deviceList elevation-1"
        dense
        v-model="selectedRows"
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
        <template v-slot:item="{ item }">
          <tr
            :class="selectedRows.indexOf(item.id) > -1 ? 'deviceRow selectedRow' : 'deviceRow'"
            @click="rowClicked($event, item)"
          >
            <td>{{ item.name }}</td>
            <td>{{ item.type }}</td>
            <td>{{ item.lastSeenAt | moment("from", "now") }}</td>
            <td>{{ item.manufacturer }}</td>
            <td>{{ item.os }}</td>
            <td>{{ item.networks }}</td>
            <td>{{ item.localAddrresses }}</td>
          </tr>
        </template>
      </v-data-table>
    </v-card>

    <v-card class="selectedItemCard" v-if="selectedRows.length">
      <v-card-title>
        Details
        <v-spacer></v-spacer>
      </v-card-title>
      <template v-for="itemId in selectedRows">
        <table v-bind:key="itemId" class="selectedDevicesSection">
          <tr>
            <td>Name</td>
            <td>{{ deviceFromID_(itemId).name }}</td>
          </tr>
          <tr>
            <td>ID</td>
            <td>{{ itemId }}</td>
          </tr>
          <tr v-if="deviceFromID_(itemId).type">
            <td>Types</td>
            <td>{{ deviceFromID_(itemId).type.join(", ") }}</td>
          </tr>
          <tr v-if="deviceFromID_(itemId).icon">
            <td>Icon</td>
            <td>{{ deviceFromID_(itemId).icon }}</td>
          </tr>
          <tr v-if="deviceFromID_(itemId).presentationURL">
            <td>Presentation</td>
            <td>{{ deviceFromID_(itemId).presentationURL }}</td>
          </tr>
          <tr v-if="deviceFromID_(itemId).operatingSystem">
            <td>OS</td>
            <td>
              {{ deviceFromID_(itemId).operatingSystem.fullVersionedName }}
            </td>
          </tr>
          <tr v-if="deviceFromID_(itemId).manufacturer">
            <td>Manufacturers</td>
            <td>
              {{
                deviceFromID_(itemId).manufacturer.fullName ||
                  deviceFromID_(itemId).manufacturer.shortName
              }}
            </td>
          </tr>
          <tr>
            <td>Networks</td>
            <td>{{ deviceFromID_(itemId).attachedNetworks }}</td>
          </tr>
          <tr>
            <td>Last Seen</td>
            <td>{{ devicexxxFromID_(itemId).lastSeenAt | moment("from", "now") }}</td>
          </tr>
          <tr v-if="deviceFromID_(itemId).debugProps">
            <td>DEBUG INFO</td>
            <td>{{ deviceFromID_(itemId).debugProps }}</td>
          </tr>
        </table>
      </template>
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

  private selectedRows: string[] = [];
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

  private clearSelection() {
    this.selectedRows = [];
  }

  private clearSelectionIfOutsideDevicesContainer() {
    this.clearSelection();
  }
  private rowClicked(e: any, row: IDevice) {
    this.toggleSelection(row.id);
    // @todo fix proper handling of shift key !e.shiftKey && !
    if (!e.ctrlKey) {
      // single select unless shift key
      this.selectedRows = this.selectedRows.filter((selectedKeyID) => selectedKeyID === row.id);
    }
    // console.log(row);
  }
  private toggleSelection(keyID: string) {
    if (this.selectedRows.includes(keyID)) {
      this.selectedRows = this.selectedRows.filter((selectedKeyID) => selectedKeyID !== keyID);
    } else {
      this.selectedRows.push(keyID);
    }
  }

  private fetchAvailableNetworks() {
    this.$store.dispatch("fetchAvailableNetworks");
  }
  private get networks(): INetwork[] {
    return this.$store.getters.getAvailableNetworks;
  }

  private formatNetworks_(attachedNetworks: { [key: string]: INetworkAttachmentInfo }): string {
    const addresses: string[] = [];
    Object.entries(attachedNetworks).forEach((element) => {
      let netID = element[0];
      this.networks.forEach((network: INetwork) => {
        if (network.id === netID) {
          // netID = network.networkAddresses.join(", ");
          // for now looks better grabbing first
          if (network.networkAddresses.length >= 1) {
            netID = network.networkAddresses[0];
          }
        }
      });

      addresses.push(netID);
    });
    return addresses.join(", ");
  }

  private formatNetworkAddresses_(attachedNetworks: {
    [key: string]: INetworkAttachmentInfo;
  }): string {
    const addresses: string[] = [];
    Object.entries(attachedNetworks).forEach((element) => {
      // console.log(element);
      element[1].networkAddresses.forEach((e: string) => addresses.push(e));
    });
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

.selectedItemCard {
  margin-top: 20px;
  margin-left: 70px;
}

.deviceList {
  margin-top: 10px;
}
.selectedDevicesSection {
  margin-top: 10px;
  margin-left: 30px;
}

.deviceRow {
  width: 100vw;
  max-width: 100vw;

  > td {
    // white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  &:hover {
    background-color: yellow !important;
  }
}

.selectedRow {
  background-color: lightyellow;
  &:hover {
    background-color: yellow !important;
  }
}
</style>
