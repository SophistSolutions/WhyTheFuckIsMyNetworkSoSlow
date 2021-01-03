<template>
  <v-container>
    <app-bar>
      <template v-slot:extrastuff>
        <v-container fluid class="extrastuff">
          <v-row no-gutters align="end">
            <v-col cols="2">
              <v-select
                dense
                hide-details="true"
                :items="selectableNetworks"
                v-model="selectedNetworkCurrent"
                label="On networks"
              />
            </v-col>
            <v-col cols="2">
              <v-select
                dense
                hide-details="true"
                :items="selectableTimeframes"
                v-model="selectedTimeframe"
                label="Seen"
              />
            </v-col>
            <v-col cols="3">
              <Search :searchFor.sync="search" dense />
            </v-col>
            <v-col cols="5" align="end">
              <FilterSummaryMessage
                dense
                :filtered="filtered"
                :nItemsSelected="filteredDevices.length"
                :nTotalItems="devices.length"
                itemsName="devices"
              />
              <ClearButton v-if="filtered" v-on:click="clearFilter" />
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
          <ReadOnlyTextWithHover
            v-if="item.lastSeenAt"
            :message="item.lastSeenAt | moment('from', 'now')"
          />
        </template>
        <template v-slot:item.type="{ headers, item }">
          <span v-for="t in ComputeDeviceTypeIconURLs(item.type)">
            <img v-if="t.url" :src="t.url" :title="t.label" height="24" width="24" />
            <ReadOnlyTextWithHover v-if="!t.url" :message="t.label" />
          </span>
        </template>
        <template v-slot:item.operatingSystem="{ headers, item }">
          <span v-for="t in ComputeOSIconURLList(item.operatingSystem)">
            <img v-if="t.url" :src="t.url" :title="t.label" height="24" width="24" />
            <ReadOnlyTextWithHover v-if="!t.url" :message="t.label" />
          </span>
        </template>
        <template v-slot:item.name="{ item }">
          <ReadOnlyTextWithHover :message="item.name" />
        </template>
        <template v-slot:item.manufacturerSummary="{ item }">
          <ReadOnlyTextWithHover :message="item.manufacturerSummary" />
        </template>
        <template v-slot:item.localAddresses="{ item }">
          <ReadOnlyTextWithHover :message="item.localAddresses" />
        </template>
        <template v-slot:item.services="{ item }">
          <span v-for="s in item.services">
            <span v-for="t in ComputeServiceTypeIconURLs([s.name])">
              <img v-if="t.url" :src="t.url" :title="t.label" height="24" width="24" />
              <ReadOnlyTextWithHover v-if="!t.url" :message="t.label" />
            </span>
          </span>
        </template>
        <template v-slot:item.networksSummary="{ item }">
          <span v-for="anw in Object.keys(item.attachedNetworks)">
            <ReadOnlyTextWithHover
              v-if="GetNetworkByIDQuietly(anw, networks)"
              :message="GetNetworkName(GetNetworkByIDQuietly(anw, networks))"
              :link="GetNetworkLink(anw)"
            />,
          </span>
        </template>
        <template v-slot:expanded-item="{ headers, item }">
          <td colspan="100">
            <Link2DetailsPage :link="'/#/device/' + item.id" />
            <DeviceDetails class="detailsSection" :device="item" :networks="networks" />
          </td>
        </template>
      </v-data-table>
    </v-card>
  </v-container>
</template>

<script lang="ts">
import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import {
  ComputeDeviceTypeIconURLs,
  ComputeOSIconURLList,
  ComputeServiceTypeIconURLs,
} from "@/models/device/Utils";
import { INetwork } from "@/models/network/INetwork";
import {
  FormatAttachedNetwork,
  FormatAttachedNetworkLocalAddresses,
  GetNetworkByID,
  GetNetworkByIDQuietly,
  GetNetworkLink,
  GetNetworkName,
  GetServices,
} from "@/models/network/Utils";
import { OperatingSystem } from "@/models/OperatingSystem";

import { Component, Prop, Vue, Watch } from "vue-property-decorator";

import { fetchNetworks } from "@/proxy/API";

@Component({
  name: "Devices",
  components: {
    AppBar: () => import("@/components/AppBar.vue"),
    ClearButton: () => import("@/components/ClearButton.vue"),
    DeviceDetails: () => import("@/components/DeviceDetails.vue"),
    FilterSummaryMessage: () => import("@/components/FilterSummaryMessage.vue"),
    Link2DetailsPage: () => import("@/components/Link2DetailsPage.vue"),
    ReadOnlyTextWithHover: () => import("@/components/ReadOnlyTextWithHover.vue"),
    Search: () => import("@/components/Search.vue"),
  },
})
export default class Devices extends Vue {
  @Prop()
  public selectedNetwork!: string | null;
  private selectedNetworkCurrent: string | null = null;
  private ComputeDeviceTypeIconURLs = ComputeDeviceTypeIconURLs;
  private ComputeServiceTypeIconURLs = ComputeServiceTypeIconURLs;
  private ComputeOSIconURLList = ComputeOSIconURLList;
  private FormatAttachedNetwork = FormatAttachedNetwork;
  private GetNetworkLink = GetNetworkLink;
  private GetNetworkByID = GetNetworkByID;
  private GetNetworkByIDQuietly = GetNetworkByIDQuietly;
  private GetNetworkName = GetNetworkName;
  private polling: undefined | number = undefined;
  private search: string = "";
  private sortBy: any = [];
  private sortDesc: any = [];
  private expanded: any[] = [];

  private get selectableNetworks(): object[] {
    const r: object[] = [
      {
        text: "Any",
        value: null,
      },
    ];
    this.networks.forEach((n) => {
      r.push({
        text: GetNetworkName(n),
        value: n.id,
      });
    });
    return r;
  }
  private get selectableTimeframes(): object[] {
    return [
      {
        text: "Ever",
        value: null,
      },
      {
        text: "Last Few Minutes",
        value: "PT2M",
      },
      {
        text: "Last Hour",
        value: "PT1H",
      },
      {
        text: "Last Day",
        value: "P1D",
      },
      {
        text: ">15 Min Ago",
        value: "-PT15M",
      },
      {
        text: ">1 Day Ago",
        value: "-P1D",
      },
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

  private fetchAvailableNetworks() {
    this.$store.dispatch("fetchAvailableNetworks");
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
        text: "Type",
        value: "type",
        cellClass: "nowrap",
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
        text: "Services",
        value: "services",
        cellClass: "nowrap",
        width: "13%",
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
    // @see https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/14
    // This works, but maybe cleaner to do within the router, but wasn't able to get
    // working (so far)
    if (this.$route.query.selectedNetwork) {
      const s: string = this.$route.query.selectedNetwork as string;
      this.$router.replace({
        path: this.$route.path,
        // params: { selectedNetwork: this.$route.query.selectedNetwork },
      });
      this.selectedNetworkCurrent = s;
    }

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

  private get filtered(): boolean {
    return (
      this.selectedNetworkCurrent !== null || this.selectedTimeframe !== null || this.search !== ""
    );
  }

  private clearFilter() {
    this.selectedNetworkCurrent = null;
    this.selectedTimeframe = null;
    this.search = "";
  }

  private get filteredDevices(): object[] {
    const result: object[] = [];
    this.devices.forEach((i) => {
      let passedFilter = true;
      if (
        passedFilter &&
        !(
          this.selectedNetworkCurrent == null ||
          i.attachedNetworks.hasOwnProperty(this.selectedNetworkCurrent)
        )
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
            localAddresses: FormatAttachedNetworkLocalAddresses(i.attachedNetworks),
            manufacturerSummary:
              i.manufacturer == null ? "" : i.manufacturer.fullName || i.manufacturer.shortName,
            services: GetServices(i),
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

.detailsSection {
  margin-top: 1em;
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
