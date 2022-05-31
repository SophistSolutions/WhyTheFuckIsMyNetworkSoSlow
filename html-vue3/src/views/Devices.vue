<script setup lang="ts">
import { defineComponent, defineProps, onMounted, nextTick, ref, computed } from 'vue';
import { useRoute, useRouter } from 'vue-router'
import { useStore } from 'vuex'
import { duration } from 'moment';

import { IDevice } from "@/models/device/IDevice";
import {
  ComputeDeviceTypeIconURLs,
  ComputeOSIconURLList,
  ComputeServiceTypeIconURLs,
} from "@/models/device/Utils";
import { INetwork } from "@/models/network/INetwork";
import {
  FormatAttachedNetworkLocalAddresses,
  GetNetworkByIDQuietly,
  GetNetworkLink,
  GetNetworkName,
  GetServices,
} from "@/models/network/Utils";

// Components
import Search from '../components/Search.vue';
import ClearButton from '../components/ClearButton.vue';
import DeviceDetails from '../components/DeviceDetails.vue';
import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';
import Link2DetailsPage from '../components/Link2DetailsPage.vue';
import FilterSummaryMessage from '../components/FilterSummaryMessage.vue';
import AppBar from "@/components/AppBar.vue";

const store = useStore()

const props = defineProps({
  selectedNetworkink: { type: String, required: false, default: null },
})


var polling: undefined | number = undefined;
var search: string = "";
var sortBy: any = [];
var sortDesc: any = [];
var expanded: any[] = [];

var selectedServices: string[] = selectableServices().map((x) => x.value);

function filterAllowAllServices() {
  return selectedServices.length === selectableServices().length;
}

function selectServicesFilter_icon() {
  if (filterAllowAllServices()) {
    return "mdi-close-box";
  }
  if (selectedServices.length > 0 && !filterAllowAllServices()) {
    return "mdi-minus-box";
  }
  return "mdi-checkbox-blank-outline";
}



function selectServicesFilter_ToggleSelectAll() {
  nextTick(() => {
    if (filterAllowAllServices()) {
      selectedServices = [];
    } else {
      selectedServices = selectableServices().map((x) => x.value);
    }
  });
}


// const count = ref(1)
// const plusOne = computed({
//   get: () => count.value + 1,
//   set: val => {
//     count.value = val - 1
//   }
// })

const selectableNetworks = computed<object[]>(
  () => {
    const r: object[] = [
      {
        title: "Any",
        value: null,
      },
    ];
    networks().forEach((n) => {
      r.push({
        title: GetNetworkName(n),
        value: n.id,
      });
    });
    return r;
  }
)
let selectedNetworkCurrent: string | undefined = undefined;

const selectableTimeframes = computed<object[]>(
  () => [
    {
      title: "Ever",
      value: null,
    },
    {
      title: "Last Few Minutes",
      value: "PT3.9M",
    },
    {
      title: "Last Hour",
      value: "PT1H",
    },
    {
      title: "Last Day",
      value: "P1D",
    },
    {
      title: ">15 Min Ago",
      value: "-PT15M",
    },
    {
      title: ">1 Day Ago",
      value: "-P1D",
    },
  ]
);
let selectedTimeframe: string | undefined = undefined;

function selectableServices(): Array<{ text: string; value: string }> {
  return [
    {
      text: "Other",
      value: "other",
    },
    {
      text: "Print",
      value: "print",
    },
    {
      text: "RDP (Remote Desktop)",
      value: "rdp",
    },
    {
      text: "SSH",
      value: "ssh",
    },
    {
      text: "SMB (Windows Network FS)",
      value: "smb",
    },
    {
      text: "Web (HTTP/HTTPS)",
      value: "web",
    },
  ];
}

function rowClicked(row: any) {
  // @todo Try this again with vue3 - https://github.com/vuetifyjs/vuetify/issues/9720
  // if (!e.ctrlKey) {
  //   // single select unless shift key
  //
  const index = expanded.indexOf(row);
  expanded = [];
  if (index === -1) {
    expanded.push(row);
  }
}

function networks(): INetwork[] {
  return store.getters.getAvailableNetworks;
}

function headers(): object[] {
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
      width: "5em",
    },
    {
      text: "Last Seen",
      value: "lastSeenAt",
      cellClass: "nowrap",
      width: "12%",
    },
    {
      text: "Manufacturer",
      value: "manufacturerSummary",
      cellClass: "nowrap",
      width: "18%",
    },
    {
      text: "OS",
      value: "operatingSystem",
      cellClass: "nowrap",
      width: "5em",
    },
    {
      text: "Services",
      value: "services",
      cellClass: "nowrap",
      width: "7em",
    },
    {
      text: "Local Address",
      value: "localAddresses",
      cellClass: "nowrap",
      width: "14%",
    },
    {
      text: "Network",
      value: "networksSummary",
      cellClass: "nowrap",
      width: "14%",
    },
    {
      text: "Details",
      value: "data-table-expand",
      width: "6em",
    },
  ];
}

const route = useRoute()
const router = useRouter()

function beforeDestroy() {
  clearInterval(polling);
}

function pollData() {
  // first time check quickly, then more gradually
  store.dispatch("fetchDevices", null);
  store.dispatch("fetchAvailableNetworks");
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    store.dispatch("fetchDevices", null);
    store.dispatch("fetchAvailableNetworks");
  }, 15 * 1000);
}

function devices(): IDevice[] {
  return store.getters.getDevices;
}

function filtered(): boolean {
  return (
    selectedNetworkCurrent != undefined ||
    selectedTimeframe !== null ||
    search !== "" ||
    !filterAllowAllServices
  );
}

function clearFilter() {
  selectedNetworkCurrent = undefined;
  selectedTimeframe = undefined;
  selectedServices = selectableServices().map((x) => x.value);
  search = "";
}

function filteredDevices(): object[] {
  const result: object[] = [];
  devices().forEach((i) => {
    let passedFilter = true;
    if (
      passedFilter &&
      !(
        selectedNetworkCurrent == null ||
        Object.prototype.hasOwnProperty.call(i.attachedNetworks, selectedNetworkCurrent)
      )
    ) {
      passedFilter = false;
    }
    if (passedFilter && selectedTimeframe !== null) {
      if (i.lastSeenAt == null) {
        // not sure how to treat missing data so for now treat as passed filter
      } else {
        const timeSinceSeenAsMS = Date.now() - new Date(i.lastSeenAt).getTime();
        const selectedTimeframeAsMS = duration(selectedTimeframe)
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
    if (passedFilter && !filterAllowAllServices()) {
      passedFilter =
        GetServices(i)
          .map((x) => x.name)
          .filter((value) => selectedServices.includes(value)).length > 0;
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
        search === "" ||
        JSON.stringify(r)
          .toLowerCase()
          .includes(search.toLowerCase())
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



defineComponent({
  components: {
    AppBar,
    ClearButton,
    ReadOnlyTextWithHover,
    Link2DetailsPage,
    Search,
  },
});

onMounted(() => {
  // @see https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/14
  // This works, but maybe cleaner to do within the router, but wasn't able to get
  // working (so far)
  if (route.query.selectedNetwork) {
    const s: string = route.query.selectedNetwork as string;
    router.replace({
      path: route.path,
      // params: { selectedNetwork: this.$route.query.selectedNetwork },
    });
    selectedNetworkCurrent = s;
  }

  pollData()
})
</script>


<template>
  <app-bar>
    <template v-slot:extrastuff>
      <v-container fluid class="extrastuff">
        <v-row no-gutters align="end">
          <v-col cols="2">
            <v-select dense hide-details="true" :items="selectableNetworks" v-model="selectedNetworkCurrent"
              label="On networks" />
          </v-col>
          <v-col cols="2">
            <v-select dense hide-details="true" :items="selectableTimeframes" v-model="selectedTimeframe"
              label="Seen" />
          </v-col>
          <v-col cols="2" v-if="false">
            <v-select dense small multiple hide-details="true"
              hint="Any means dont filter on this; multiple selected items treated as OR" :items="selectableServices()"
              v-model="selectedServices" :menu-props="{ auto: true, overflowY: true }" label="With services">
              <template v-slot:prepend-item>
                <v-list-item ripple @click="selectServicesFilter_ToggleSelectAll">
                  <v-list-item-action>
                    <v-icon :color="selectedServices.length > 0 ? 'indigo darken-4' : ''">
                      {{ selectServicesFilter_icon }}
                    </v-icon>
                  </v-list-item-action>
                  <v-list-item-title>
                    Any
                  </v-list-item-title>

                </v-list-item>
                <v-divider class="mt-2"></v-divider>
              </template>

              <template slot="selection" slot-scope="{ item, index }">
                <span v-if="filterAllowAllServices() && index === 0">Any</span>
                <span v-if="index === 0 && !filterAllowAllServices()">{{ item.text }}</span>
                <span v-if="index === 1 && !filterAllowAllServices" class="grey--text caption othersSpan">(+{{
                    selectedServices.length - 1
                }} others)</span>
              </template>
            </v-select>
          </v-col>
          <v-col cols="2" v-if="false">
            <Search :searchFor.sync="search" dense />
          </v-col>
          <v-col cols="4" align="end" v-if="false">
            <FilterSummaryMessage dense :filtered="filtered" :nItemsSelected="filteredDevices().length"
              :nTotalItems="devices().length" itemsName="devices" />
            <ClearButton v-if="false /*filtered*/" v-on:click="clearFilter" />
          </v-col>
        </v-row>
      </v-container>
    </template>
  </app-bar>

  <v-container v-if="null">
    <v-card class="deviceListCard">
      <v-card-title>
        Devices
        <v-spacer></v-spacer>
      </v-card-title>
      <v-table fixed-header class="deviceList elevation-1" dense show-expand :expanded.sync="expanded"
        :single-expand="true" :headers="headers" :items="filteredDevices()" :single-select="true" :sort-by="sortBy"
        :sort-desc="sortDesc" multi-sort disable-pagination :hide-default-footer="true" item-key="id"
        @click:row="rowClicked">
        <template v-slot:[`item.lastSeenAt`]="{ item }">
          <ReadOnlyTextWithHover v-if="item.lastSeenAt" :message="item.lastSeenAt | moment('from', 'now')" />
        </template>
        <template v-slot:[`item.type`]="{ item }">
          <span v-for="(t, i) in ComputeDeviceTypeIconURLs(item.type)" :key="i">
            <img v-if="t.url" :src="t.url" :title="t.label" height="20" width="20" />
            <ReadOnlyTextWithHover v-if="!t.url" :message="t.label" />
          </span>
        </template>
        <template v-slot:[`item.operatingSystem`]="{ item }">
          <span v-for="(t, i) in ComputeOSIconURLList(item.operatingSystem)" :key="i">
            <img v-if="t.url" :src="t.url" :title="t.label" height="20" width="20" />
            <ReadOnlyTextWithHover v-if="!t.url" :message="t.label" />
          </span>
        </template>
        <template v-slot:[`item.name`]="{ item }">
          <ReadOnlyTextWithHover :message="item.name" />
        </template>
        <template v-slot:[`item.manufacturerSummary`]="{ item }">
          <ReadOnlyTextWithHover :message="item.manufacturerSummary" />
        </template>
        <template v-slot:[`item.localAddresses`]="{ item }">
          <ReadOnlyTextWithHover :message="item.localAddresses" />
        </template>
        <template v-slot:[`item.services`]="{ item }">
          <span v-for="(s, i) in item.services" :key="i">
            <span v-for="(t, i) in ComputeServiceTypeIconURLs([s.name])" :key="i">
              <img v-if="t.url" :src="t.url" :title="t.label" height="20" width="20" />
              <ReadOnlyTextWithHover v-if="!t.url" :message="t.label" />
            </span>
          </span>
        </template>
        <template v-slot:[`item.networksSummary`]="{ item }">
          <span v-for="(anw, i) in Object.keys(item.attachedNetworks)" :key="i">
            <ReadOnlyTextWithHover v-if="GetNetworkByIDQuietly(anw, networks)"
              :message="GetNetworkName(GetNetworkByIDQuietly(anw, networks))" :link="GetNetworkLink(anw)" />&nbsp;
          </span>
        </template>
        <template v-slot:expanded-item="{ item }">
          <td colspan="100">
            <Link2DetailsPage :link="'/#/device/' + item.id" />
            <DeviceDetails class="detailsSection" :deviceId="item.id" />
          </td>
        </template>
      </v-table>
    </v-card>
  </v-container>
</template>



<style lang="scss">
.deviceList>div>table {
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
