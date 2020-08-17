<template>
  <v-container class="devices" v-click-outside="clearSelectionIfOutsideDevicesContainer">
    <v-card>
      <v-card-title>
        Networks
        <v-spacer></v-spacer>
      </v-card-title>
      <v-data-table
        height="2in"
        class="deviceList"
        dense
        v-model="selectedRows"
        :headers="headers"
        :items="networksAsDisplayed"
        :single-select="true"
        :search="search"
        :sort-by="sortBy"
        :sort-desc="sortDesc"
        multi-sort
        disable-pagination
        :hide-default-footer="true"
        item-key="id"
        xclass="elevation-1"
      >
        <template v-slot:item="{ item }">
          <tr
            :class="selectedRows.indexOf(item.id) > -1 ? 'yellow' : ''"
            @click="rowClicked($event, item)"
          >
            <td>{{ item.name }}</td>
            <td>{{ item.active }}</td>
            <td>{{ item.status }}</td>
            <td>{{ item.location }}</td>
            <td>{{ item.internetInfo }}</td>
            <td>{{ item.devices }}</td>
          </tr>
        </template>
      </v-data-table>
    </v-card>
    <v-card v-if="selectedRows.length">
      <v-card-title>
        Details
        <v-spacer></v-spacer>
      </v-card-title>
      <template v-for="itemId in selectedRows">
        <table v-bind:key="itemId" class="selectedDevicesSection">
          <tr v-bind:key="itemId">
            <td>Name</td>
            <td>{{ networkFromID_(itemId).name }}</td>
          </tr>
          <tr v-bind:key="itemId">
            <td>ID</td>
            <td>{{ itemId }}</td>
          </tr>
          <tr v-bind:key="itemId" v-if="networkFromID_(itemId).DNSServers">
            <td>DNSServers</td>
            <td>{{ networkFromID_(itemId).DNSServers.join(", ") }}</td>
          </tr>
          <tr v-bind:key="itemId" v-if="networkFromID_(itemId).gateways">
            <td>gateways</td>
            <td>{{ networkFromID_(itemId).gateways.join(", ") }}</td>
          </tr>
          <tr v-bind:key="itemId" v-if="networkFromID_(itemId).geographicLocation">
            <td>geographicLocation</td>
            <td>{{ networkFromID_(itemId).geographicLocation }}</td>
          </tr>
          <tr v-bind:key="itemId" v-if="networkFromID_(itemId).internetServiceProvider">
            <td>internetServiceProvider</td>
            <td>{{ networkFromID_(itemId).internetServiceProvider }}</td>
          </tr>
          <tr v-bind:key="itemId" v-if="networkFromID_(itemId).debugProps">
            <td>DEBUG INFO</td>
            <td>{{ networkFromID_(itemId).debugProps }}</td>
          </tr>
        </table>
      </template>
    </v-card>
  </v-container>
</template>

<script lang="ts">
import { INetwork } from "@/models/network/INetwork";
import { Component, Vue, Watch } from "vue-property-decorator";

import { compareValues } from "@/CustomSort";
import { fetchNetworks } from "@/proxy/API";

@Component({
  name: "Networks",
  components: {},
})
export default class Devices extends Vue {
  private polling: undefined | number = undefined;

  private selectedRows: string[] = [];
  private sortBy: any = [];
  private sortDesc: any = [];

  // terrible inefficient approach - maybe create map object dervied from devices array
  private networkFromID_(id: string) {
    let result = null;
    this.networks.every((d) => {
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
  private rowClicked(e: any, row: INetwork) {
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

  // private formatNetworks_(attachedNetworks: { [key: string]: INetworkAttachmentInfo }): string {
  //   const addresses: string[] = [];
  //   Object.entries(attachedNetworks).forEach((element) => {
  //     // console.log(element);
  //     addresses.push(element[0]);
  //   });
  //   return addresses.join(", ");
  // }

  // private formatNetworkAddresses_(attachedNetworks: {
  //   [key: string]: INetworkAttachmentInfo;
  // }): string {
  //   const addresses: string[] = [];
  //   Object.entries(attachedNetworks).forEach((element) => {
  //     // console.log(element);
  //     element[1].networkAddresses.forEach((e: string) => addresses.push(e));
  //   });
  //   return addresses.join(", ");
  // }

  private fetchAvailableNetworks() {
    this.$store.dispatch("fetchAvailableNetworks");
  }

  private pollData() {
    this.polling = setInterval(() => {
      this.fetchAvailableNetworks();
    }, 10 * 1000);
  }

  private get networks(): INetwork[] {
    return this.$store.getters.getAvailableNetworks;
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
        text: "Active",
        value: "active",
      },
      {
        text: "Status",
        value: "status",
      },
      {
        text: "Location",
        value: "location",
      },
      {
        text: "Internet",
        value: "internetInfo",
      },
      {
        text: "devices",
        value: "devices",
      },
    ];
  }

  private created() {
    this.fetchAvailableNetworks();
    this.pollData();
  }

  private beforeDestroy() {
    clearInterval(this.polling);
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
        devices: "19",
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
}
</script>

<style lang="scss">
.devices {
  position: relative;
  max-width: 90%;
  margin: auto;
  margin-top: 20px;
}

.deviceList {
  margin-top: 10px;
}
.selectedDevicesSection {
  margin-top: 10px;
  margin-left: 10px;
  margin-right: 10px;
  margin-bottom: 10px;
}

.flip-list-move {
  transition: transform 500ms;
}
</style>
