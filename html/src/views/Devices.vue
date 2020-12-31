<template>
  <v-container>
    <app-bar>
      <template v-slot:extrastuff>
        <v-container fluid class="extrastuff">
          <v-row no-gutters align="end">
            <v-col>
              <v-select
                dense
                hide-details="true"
                :items="selectableNetworks"
                v-model="selectedNetwork"
                label="On networks"
              />
            </v-col>
            <v-col>
              <v-select
                dense
                hide-details="true"
                :items="selectableTimeframes"
                v-model="selectedTimeframe"
                label="Seen"
              />
            </v-col>
            <v-col>
              <Search :searchFor.sync="search" dense />
            </v-col>
            <v-col>
              <FilterSummaryMessage
                dense
                :nItemsSelected="filteredDevices.length"
                :nTotalItems="devices.length"
                itemsName="devices"
              />
            </v-col>
          </v-row>
        </v-container>
      </template>
    </app-bar>

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
        :items="filteredDevices"
        :single-select="true"
        :sort-by="sortBy"
        :sort-desc="sortDesc"
        multi-sort
        disable-pagination
        :hide-default-footer="true"
        item-key="id"
        @click:row="rowClicked"
      >
        <template v-slot:item.lastSeenAt="{ headers, item }">
          <ReadOnlyTextWithTitle
            v-if="item.lastSeenAt"
            :message="item.lastSeenAt | moment('from', 'now')"
          />
        </template>
        <template v-slot:item.type="{ headers, item }">
          <span v-for="t in computeDeviceTypeIconURLs_(item.type)">
            <img v-if="t.url" :src="t.url" :title="t.label" height="24" width="24" />
            <ReadOnlyTextWithTitle v-if="!t.url" :message="t.label" />
          </span>
        </template>
        <template v-slot:item.operatingSystem="{ headers, item }">
          <span v-for="t in computeOSIconURLList_(item.operatingSystem)">
            <img v-if="t.url" :src="t.url" :title="t.label" height="24" width="24" />
            <ReadOnlyTextWithTitle v-if="!t.url" :message="t.label" />
          </span>
        </template>
        <template v-slot:item.name="{ item }">
          <ReadOnlyTextWithTitle :message="item.name" />
        </template>
        <template v-slot:item.manufacturerSummary="{ item }">
          <ReadOnlyTextWithTitle :message="item.manufacturerSummary" />
        </template>
        <template v-slot:item.localAddresses="{ item }">
          <ReadOnlyTextWithTitle :message="item.localAddresses" />
        </template>
        <template v-slot:item.networksSummary="{ item }">
          <ReadOnlyTextWithTitle :message="item.networksSummary" />
        </template>
        <template v-slot:expanded-item="{ headers, item }">
          <td colspan="100">
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
import { GetNetworkName } from "@/models/network/Utils";
import { OperatingSystem } from "@/models/OperatingSystem";

import { Component, Vue, Watch } from "vue-property-decorator";

import { fetchNetworks } from "@/proxy/API";

@Component({
  name: "Devices",
  components: {
    AppBar: () => import("@/components/AppBar.vue"),
    DeviceDetails: () => import("@/components/DeviceDetails.vue"),
    FilterSummaryMessage: () => import("@/components/FilterSummaryMessage.vue"),
    ReadOnlyTextWithTitle: () => import("@/components/ReadOnlyTextWithTitle.vue"),
    Search: () => import("@/components/Search.vue"),
  },
})
export default class Devices extends Vue {
  private polling: undefined | number = undefined;
  private search: string = "";
  private sortBy: any = [];
  private sortDesc: any = [];
  private expanded: any[] = [];
  private selectedNetwork: string | null = null;
  private get selectableNetworks(): object[] {
    const r: object[] = [{ text: "Any", value: null }];
    this.networks.forEach((n) => {
      r.push({ text: GetNetworkName(n), value: n.id });
    });
    return r;
  }
  private get selectableTimeframes(): object[] {
    return [
      { text: "Ever", value: null },
      { text: "Last Few Minutes", value: "PT2M" },
      { text: "Last Hour", value: "PT1H" },
      { text: "Last Day", value: "P1D" },
      { text: ">15 Min Ago", value: "-PT15M" },
      { text: ">1 Day Ago", value: "-P1D" },
    ];
  }
  private selectedTimeframe: string | null = null;

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
      element[1].networkAddresses.forEach((e: string) => addresses.push(e));
    });
    addresses = addresses.filter((value, index, self) => self.indexOf(value) === index);
    return addresses.join(", ");
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
        text: "Type",
        value: "type",
        width: "10%",
      },
      {
        text: "Last Seen",
        value: "lastSeenAt",
        cellClass: "nowrap",
        width: "14%",
      },
      {
        text: "Manufacturer",
        value: "manufacturerSummary",
        cellClass: "nowrap",
        width: "20%",
      },
      {
        text: "OS",
        value: "operatingSystem",
        cellClass: "nowrap",
        width: "8%",
      },
      {
        text: "Local Address",
        value: "localAddresses",
        cellClass: "nowrap",
        width: "20%",
      },
      {
        text: "Network",
        value: "networksSummary",
        cellClass: "nowrap",
        width: "15%",
      },
      {
        text: "Details",
        value: "data-table-expand",
        width: "10%",
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

  private get filteredDevices(): object[] {
    const result: object[] = [];
    this.devices.forEach((i) => {
      let passedFilter = true;
      if (
        passedFilter &&
        !(this.selectedNetwork == null || i.attachedNetworks.hasOwnProperty(this.selectedNetwork))
      ) {
        passedFilter = false;
      }
      if (passedFilter && this.selectedTimeframe !== null) {
        if (i.lastSeenAt == null) {
          // not sure how to treat missing data so for now treat as passed filter
        } else {
          const timeSinceSeenAsMS = Date.now() - new Date(i.lastSeenAt).getTime();
          const selectedTimeframeAsMS = this.$moment
            .duration(this.selectedTimeframe)
            .asMilliseconds();
          if (selectedTimeframeAsMS < 0) {
            // Treat negative duration as meaning stuff OLDER than
            if (-selectedTimeframeAsMS > timeSinceSeenAsMS) {
              passedFilter = false;
            }
          } else {
            if (selectedTimeframeAsMS < timeSinceSeenAsMS) {
              passedFilter = false;
            }
          }
        }
      }
      if (passedFilter) {
        const r = {
          ...i,
          ...{
            networksSummary: this.formatNetworks_(i.attachedNetworks),
            localAddresses: this.formatNetworkAddresses_(i.attachedNetworks),
            manufacturerSummary:
              i.manufacturer == null ? "" : i.manufacturer.fullName || i.manufacturer.shortName,
          },
        };
        if (
          this.search === "" ||
          JSON.stringify(r)
            .toLowerCase()
            .includes(this.search.toLowerCase())
        ) {
          result.push(r);
        }
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
.deviceList > div > table {
  table-layout: fixed;
  //background-color: red;
}
.nowrap {
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.deviceListCard {
  margin-top: 10px;
  margin-left: 10px;
}

.deviceList {
  margin-top: 10px;
}

.extrastuff {
  padding: 0 12px;
  border: 4px red;
  // background-color: black;
}
</style>
