<template>
  <v-container class="devices" v-click-outside="clearSelectionIfOutsideDevicesContainer">
    <v-card>
      <v-card-title>
        Networks
        <v-spacer></v-spacer>
      </v-card-title>
      <v-data-table
        height="2in"
        class="networkList"
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
            <td>
              <router-link to="/devices">{{ item.devices }}</router-link>
            </td>
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
        <div class="selectedDevicesSection">
          <NetworkDetails :network="networkFromID_(itemId)" :devices="devices"></NetworkDetails>
        </div>
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
  name: "Networks",
  components: {
    NetworkDetails: () => import("@/components/NetworkDetails.vue"),
  },
})
export default class Networks extends Vue {
  private polling: undefined | number = undefined;

  private selectedRows: string[] = [];
  private sortBy: any = [];
  private sortDesc: any = [];

  // terrible inefficient approach - maybe create map object dervied from devices array
  private networkFromID_(id: string): INetwork | null {
    let result: INetwork | null = null;
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
}
</script>

<style lang="scss">
.devices {
  position: relative;
  max-width: 90%;
  margin: auto;
  margin-top: 20px;
}

.networkList {
  margin-top: 10px;
}
.selectedDevicesSection {
  margin-left: 40px;
  margin-right: 10px;
  margin-bottom: 10px;
}

.flip-list-move {
  transition: transform 500ms;
}
</style>
