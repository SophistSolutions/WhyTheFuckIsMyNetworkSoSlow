<template>
  <v-container class="devices">
    <app-bar>
      <template v-slot:extrastuff>
        <v-container class="extrastuff">
          <v-row no-gutters dense align="end">
            <v-col cols="3">
              <Search dense :searchFor.sync="search" />
            </v-col>
            <v-col cols="9" align="end">
              <FilterSummaryMessage
                dense
                :filtered="filtered"
                :nItemsSelected="filteredNetworks.length"
                :nTotalItems="networks.length"
                itemsName="networks"
              />
              <ClearButton v-if="filtered" v-on:click="clearFilter" />
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
        <template v-slot:[`item.devices`]="{ item }">
          <td>
            <a :href="GetDevicesForNetworkLink(item.id)">{{ item.devices }}</a>
          </td>
        </template>
        <template v-slot:[`item.name`]="{ item }">
          <ReadOnlyTextWithHover :message="GetNetworkName(item)" :link="GetNetworkLink(item)" />
        </template>
        <template v-slot:[`item.location`]="{ item }">
          <ReadOnlyTextWithHover :message="item.location" />
        </template>
        <template v-slot:[`item.internetInfo`]="{ item }">
          <ReadOnlyTextWithHover :message="item.internetInfo" />
        </template>
        <template v-slot:expanded-item="{ item }">
          <td colspan="100">
            <Link2DetailsPage :link="'/#/network/' + item.id" />
            <NetworkDetails
              class="detailsSection"
              :network="networkFromID_(item.id)"
              :devices="devices"
            ></NetworkDetails>
          </td>
        </template>
      </v-data-table>
    </v-card>
  </v-container>
</template>

<script lang="ts">
import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import { INetwork } from "@/models/network/INetwork";
import {
  FormatLocation,
  GetDeviceIDsInNetwork,
  GetDevicesForNetworkLink,
  GetNetworkCIDRs,
  GetNetworkLink,
  GetNetworkName,
} from "@/models/network/Utils";
import { Component, Vue, Watch } from "vue-property-decorator";

import { fetchNetworks } from "@/proxy/API";

@Component({
  name: "Networks",
  components: {
    AppBar: () => import("@/components/AppBar.vue"),
    ClearButton: () => import("@/components/ClearButton.vue"),
    FilterSummaryMessage: () => import("@/components/FilterSummaryMessage.vue"),
    Link2DetailsPage: () => import("@/components/Link2DetailsPage.vue"),
    NetworkDetails: () => import("@/components/NetworkDetails.vue"),
    ReadOnlyTextWithHover: () => import("@/components/ReadOnlyTextWithHover.vue"),
    Search: () => import("@/components/Search.vue"),
  },
})
export default class Networks extends Vue {
  private polling: undefined | number = undefined;

  private GetNetworkLink = GetNetworkLink;
  private GetNetworkName = GetNetworkName;
  private GetDevicesForNetworkLink = GetDevicesForNetworkLink;

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
        cellClass: "nowrap",
        width: "20%",
      },
      {
        text: "CIDRs",
        align: "start",
        value: "CIDRs",
        cellClass: "nowrap",
        width: "10%",
      },
      {
        text: "Active",
        value: "active",
        cellClass: "nowrap",
        width: "10%",
      },
      {
        text: "Status",
        value: "status",
        cellClass: "nowrap",
        width: "10%",
      },
      {
        text: "Location",
        value: "location",
        cellClass: "nowrap",
        width: "20%",
      },
      {
        text: "Internet",
        value: "internetInfo",
        cellClass: "nowrap",
        width: "20%",
      },
      {
        text: "devices",
        value: "devices",
        cellClass: "nowrap",
        width: "10%",
      },
      {
        text: "Details",
        value: "data-table-expand",
        width: "6em",
      },
    ];
  }

  private get devices(): IDevice[] {
    return this.$store.getters.getDevices;
  }

  private get filtered(): boolean {
    return this.search !== "";
  }
  private clearFilter() {
    this.search = "";
  }

  private get filteredNetworks(): object[] {
    const result: object[] = [];
    this.networks.forEach((i) => {
      let lastSeenStr = this.$moment(i.lastSeenAt).fromNow();
      let status = "?";
      if (i.lastSeenAt != null && this.$moment().diff(this.$moment(i.lastSeenAt), "seconds") < 60) {
        lastSeenStr = "active";
        status = "healthy"; // tmphack
      }
      const r: any = {
        ...i,
        name: GetNetworkName(i),
        CIDRs: GetNetworkCIDRs(i),
        active: lastSeenStr,
        internetInfo:
          (i.gateways == null ? "" : i.gateways.join(", ")) +
          (i.internetServiceProvider == null ? " " : " (" + i.internetServiceProvider.name + ")"),
        devices: GetDeviceIDsInNetwork(i, this.devices).length,
        status: status,
        location: FormatLocation(i.geographicLocation),
      };
      if (
        this.search === "" ||
        JSON.stringify(r)
          .toLowerCase()
          .includes(this.search.toLowerCase())
      ) {
        // console.log("MATCH: this.search=", this.search, " and r=", JSON.stringify(r));
        result.push(r);
      } else {
        // console.log("NO MATCH: this.search=", this.search, " and r=", JSON.stringify(r));
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

.detailsSection {
  margin-top: 1em;
}

.networkList {
  margin-top: 10px;
}

.networkList > div > table {
  table-layout: fixed;
  //background-color: red;
}

.nowrap {
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
</style>
