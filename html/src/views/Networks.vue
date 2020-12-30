<template>
  <v-container class="devices">
    <app-bar>
      <template v-slot:extrastuff>
        <v-container fluid class="extrastuff">
          <v-row no-gutters align="end">
            <v-col>
              <Search dense :searchFor.sync="search" />
            </v-col>
            <v-col>
              <!-- <v-col style="display: flex; flex-direction: column; justify-content: space-between;;"> -->
              <!-- <div /> -->
              <FilterSummaryMessage
                dense
                :nItemsSelected="filteredNetworks.length"
                :nTotalItems="networks.length"
                itemsName="networks"
              />
            </v-col>
          </v-row>
        </v-container>
      </template>
    </app-bar>
    <v-card>
      <v-card-title>
        Networks
        <v-spacer></v-spacer>
      </v-card-title>
      <v-data-table
        class="networkList"
        dense
        show-expand
        :expanded.sync="expanded"
        :single-expand="true"
        :headers="headers"
        :items="filteredNetworks"
        :single-select="true"
        :sort-by="sortBy"
        :sort-desc="sortDesc"
        multi-sort
        disable-pagination
        :hide-default-footer="true"
        item-key="id"
        @click:row="rowClicked"
      >
        <template v-slot:item.devices="{ headers, item }">
          <td>
            <router-link to="/devices">{{ item.devices }}</router-link>
          </td>
        </template>
        <template v-slot:expanded-item="{ headers, item }">
          <td :colspan="headers.length">
            <NetworkDetails :network="networkFromID_(item.id)" :devices="devices"></NetworkDetails>
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

import { fetchNetworks } from "@/proxy/API";

@Component({
  name: "Networks",
  components: {
    AppBar: () => import("@/components/AppBar.vue"),
    FilterSummaryMessage: () => import("@/components/FilterSummaryMessage.vue"),
    NetworkDetails: () => import("@/components/NetworkDetails.vue"),
    Search: () => import("@/components/Search.vue"),
  },
})
export default class Networks extends Vue {
  private polling: undefined | number = undefined;

  private search: string = "";
  private sortBy: any = [];
  private sortDesc: any = [];
  private expanded: any[] = [];

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
      {
        text: "Details",
        value: "data-table-expand",
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

  private get filteredNetworks(): object[] {
    const result: object[] = [];
    this.networks.forEach((i) => {
      const location: string | null =
        i.geographicLocation == null
          ? null
          : i.geographicLocation.city + ", " + i.geographicLocation.regionCode;
      const r: any = {
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
      };
      if (
        this.search === "" ||
        JSON.stringify(r)
          .toLowerCase()
          .includes(this.search.toLowerCase())
      ) {
        console.log("MATCH: this.search=", this.search, " and r=", JSON.stringify(r));
        result.push(r);
      } else {
        console.log("NO MATCH: this.search=", this.search, " and r=", JSON.stringify(r));
      }
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

.extrastuff {
  padding: 0 12px;
  //background-color: red;
}
.networkList {
  margin-top: 10px;
}
.selectedDevicesSection {
  margin-left: 40px;
  margin-right: 10px;
  margin-bottom: 10px;
}
</style>
