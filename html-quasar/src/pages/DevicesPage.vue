<script setup lang="ts">
import { defineComponent, defineProps, onMounted, onUnmounted, nextTick, ref, computed, ComputedRef } from 'vue';
import { useRoute, useRouter } from 'vue-router'
import * as moment from 'moment';

import { IDevice } from "../models/device/IDevice";
import {
  ComputeDeviceTypeIconURLs,
  ComputeOSIconURLList,
  ComputeServiceTypeIconURLs,
} from "../models/device/Utils";
import { INetwork } from "../models/network/INetwork";
import {
  FormatAttachedNetworkLocalAddresses,
  GetNetworkByIDQuietly,
  GetNetworkLink,
  GetNetworkName,
  GetServices,
} from "../models/network/Utils";
import { useQuasar } from 'quasar';

// Components
// import Search from '../components/Search.vue';
//import ClearButton from '../components/ClearButton.vue';
import DeviceDetails from '../components/DeviceDetails.vue';
import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';
import Link2DetailsPage from '../components/Link2DetailsPage.vue';
import FilterSummaryMessage from '../components/FilterSummaryMessage.vue';

import { useWTFStore } from '../stores/WTF-store'
const $q = useQuasar()

const store = useWTFStore()

const props = defineProps({
  selectedNetworkink: { type: String, required: false, default: null },
})


let polling: undefined | NodeJS.Timeout;
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

const selectableNetworks = computed<object[]>(
  () => {
    const r: object[] = [
      {
        title: "Any",
        value: null,
      },
    ];
    allAvailableNetworks.value.forEach((n) => {
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

let allAvailableNetworks: ComputedRef<INetwork[]> = computed(() => store.getAvailableNetworks);

const headers = ref([
  {
    name: 'name',
    field: "name",
    label: "Name",
    sortable: true,
    classes: "nowrap",
    align: "left",
    headerStyle: 'width: 20%; ',
  },
  {
    name: "type",
    field: "type",
    label: "Type",
    sortable: true,
    classes: "nowrap",
    align: "left",
    headerStyle: 'width: 5em',
  },
  {
    name: "lastSeenAt",
    field: "lastSeenAt",
    label: "Last Seen",
    classes: "nowrap",
    sortable: true,
    align: "left",
  },
  {
    name: "manufacturerSummary",
    field: "manufacturerSummary",
    label: "Manufacturer",
    classes: "nowrap",
    sortable: true,
    align: "left",
    headerStyle: 'width: 18%; ',
  },
  {
    name: "operatingSystem",
    field: "operatingSystem",
    label: "OS",
    classes: "nowrap",
    sortable: true,
    align: "left",
    headerStyle: 'width: 5em',
  },
  // when shown services issues vue error - not clear why...
  {
    name: "services",
    field: "services",
    label: "Services",
    classes: "nowrap",
    sortable: true,
    align: "center",
    headerStyle: 'width: 7em',
  },
  {
    name: "localAddresses",
    field: "localAddresses",
    label: "Local Address",
    classes: "nowrap",
    sortable: true,
    align: "left",
    headerStyle: 'width: 14%; ',
  },
  {
    name: "networksSummary",
    field: "attachedNetworks",
    label: "Network",
    classes: "nowrap",
    align: "left",
    sortable: true,
    headerStyle: 'width: 14%; ',
  },
  {
    name: 'expand',
    label: "Details",
    align: 'center',
    headerStyle: 'width: 6em; ',
  },
]);

let visibleColumns = ref(['name', 'type', 'lastSeenAt', 'manufacturerSummary', 'operatingSystem', 'services', 'localAddresses', 'networksSummary', 'expand']);


const route = useRoute()
const router = useRouter()

let allDevices: ComputedRef<IDevice[]> = computed(() => store.getDevices);


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


let filteredExtendedDevices: ComputedRef<object[]> = computed(() => {
  const result: object[] = [];
  allDevices.value.forEach((i) => {
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
        const selectedTimeframeAsMS = moment.duration(selectedTimeframe)
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
    passedFilter = true;
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
});

defineComponent({
  components: {
    // ClearButton,
    ReadOnlyTextWithHover,
    Link2DetailsPage,
    DeviceDetails,
    // Search,
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

  // first time check quickly, then more gradually
  store.fetchDevices();
  store.fetchAvailableNetworks();
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    store.fetchDevices();
    store.fetchAvailableNetworks();
  }, 15 * 1000);
})

onUnmounted(() => {
  clearInterval(polling);
})

// disable pagination
const pagination = ref({
  page: 1,
  rowsPerPage: 0
})
</script>

<template>
  <q-page class="col q-pa-md q-gutter-md">

    <!-- <app-bar>
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
            <FilterSummaryMessage dense :filtered="filtered" :nItemsSelected="filteredExtendedDevices.value.length"
              :nTotalItems="allDevices.value.length" itemsName="devices" />
            <ClearButton v-if="false /*filtered*/" v-on:click="clearFilter" />
          </v-col>
        </v-row>
      </v-container>
    </template>
  </app-bar> -->

    <q-card class="deviceListCard">
      <q-card-section>
        <div class="row text-h5">
          Devices
        </div>
        <q-table id="xxx" table-class="deviceList shadow-1" :rows="filteredExtendedDevices" :columns="headers"
          row-key="id" separator="none" :visible-columns="visibleColumns" :pagination.sync="pagination" hide-bottom>

          <!--@todo migrate to extrastuff slot in filter section above-->
          <template v-slot:top>
            <q-space />
            <q-select v-model="visibleColumns" multiple outlined dense options-dense
              :display-value="$q.lang.table.columns" emit-value map-options :options="headers" option-value="name"
              options-cover style="min-width: 150px" />
          </template>

          <template v-slot:body="props">
            <q-tr :props="props">
              <q-td :props="props" key="name">
                <ReadOnlyTextWithHover :message="props.row.name" />
              </q-td>
              <q-td :props="props" key="type">
                <span v-for="(t, i) in ComputeDeviceTypeIconURLs(props.row.type)" :key="i">
                  <img v-if="t.url" :src="t.url" :title="t.label" height="20" width="20" />
                  <ReadOnlyTextWithHover v-if="!t.url" :message="t.label" />
                </span>
              </q-td>
              <q-td :props="props" key="lastSeenAt">
                <ReadOnlyTextWithHover v-if="props.row.lastSeenAt" :message="moment(props.value).fromNow()" />
              </q-td>
              <q-td :props="props" key="manufacturerSummary">
                <ReadOnlyTextWithHover :message="props.row.manufacturerSummary" />
              </q-td>
              <q-td :props="props" key="operatingSystem">
                <span v-for="(t, i) in ComputeOSIconURLList(props.row.operatingSystem)" :key="i">
                  <img v-if="t.url" :src="t.url" :title="t.label" height="20" width="20" />
                  <ReadOnlyTextWithHover v-if="!t.url" :message="t.label" />
                </span>
              </q-td>
              <q-td :props="props" key="services">
                <span v-for="(s, i) in props.row.services" :key="i">
                  <span v-for="(t, i) in ComputeServiceTypeIconURLs([s.name])" :key="i">
                    <img v-if="t.url" :src="t.url" :title="t.label" height="20" width="20" />
                    <ReadOnlyTextWithHover v-if="!t.url" :message="t.label" />
                  </span>
                </span>
              </q-td>
              <q-td :props="props" key="localAddresses">
                <ReadOnlyTextWithHover :message="props.row.localAddresses" />
              </q-td>
              <q-td :props="props" key="networksSummary">
                <span v-for="(anw, i) in Object.keys(props.row.attachedNetworks)" :key="i">
                  <ReadOnlyTextWithHover v-if="GetNetworkByIDQuietly(anw, allAvailableNetworks)"
                    :message="GetNetworkName(GetNetworkByIDQuietly(anw, allAvailableNetworks))"
                    :link="GetNetworkLink(anw)" />&nbsp;
                </span>
              </q-td>
              <q-td :props="props" key="expand">
                <q-btn :icon="props.expand ? 'mdi-chevron-up' : 'mdi-chevron-down'" flat round dense
                  @click="props.expand = !props.expand"></q-btn>
              </q-td>
            </q-tr>
            <q-tr v-if="props.expand" :props="props">
              <q-td colspan="100%">
                <Link2DetailsPage :link="'/#/device/' + props.row.id" />
                <DeviceDetails class="detailsSection" :deviceId="props.row.id" />
              </q-td>
            </q-tr>
          </template>
        </q-table>
      </q-card-section>
    </q-card>
  </q-page>

</template>


<style lang="scss" >
.deviceListCard table {
  table-layout: fixed;
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
