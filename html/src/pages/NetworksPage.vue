<script setup lang="ts">
import { defineComponent, defineProps, onMounted, onUnmounted, nextTick, ref, computed, ComputedRef } from 'vue';
import { useRoute, useRouter } from 'vue-router'
import moment from 'moment';
import { useQuasar } from 'quasar';
import { useStorage } from '@vueuse/core'

import { IDevice } from "../models/device/IDevice";
import { INetwork } from "../models/network/INetwork";
import {
  GetNetworkName,
  GetNetworkCIDRs,
  FormatLocation,
  GetDeviceIDsInNetwork,
  GetDevicesForNetworkLink,
} from "../models/network/Utils";

// Components
import Search from '../components/Search.vue';
import ClearButton from '../components/ClearButton.vue';
import NetworkDetails from '../components/NetworkDetails.vue';
import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';
import Link2DetailsPage from '../components/Link2DetailsPage.vue';
import FilterSummaryMessage from '../components/FilterSummaryMessage.vue';

import { useNetStateStore } from '../stores/Net-State-store'
const $q = useQuasar()

const store = useNetStateStore()

const props = defineProps({
  selectedNetworkink: { type: String, required: false, default: null },
})

let polling: undefined | NodeJS.Timeout;
var search = ref("");
var sortBy: any = [];
var sortDesc: any = [];
var expanded: any[] = [];


// store page options in local storage, but eventually add many more options here (like collapsed or open show filter section etc)
// COULD POSSIBLY also store stuff like selected filter (search string etc) - but no need for now....
const pageUserOptions = useStorage('Networks-Page-User-Options', {
  VisibleColumns: ['name',
    'CIDRs',
    'active',
    'status',
    'location',
    'internetInfo',
    'devices',
    'expand']
})


function filterAllowAllServices() {
  return selectedServices.length === selectableServices.value.length;
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
      selectedServices = selectableServices.value.map((x) => x.value);
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

const selectableTimeframes = ref<object[]>(
  [
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

const selectableServices = ref<Array<{ text: string; value: string }>>(
  [
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
  ]
);

var selectedServices: string[] = selectableServices.value.map((x) => x.value);

function rowClicked(props: object) {
  props.expand = !props.expand;
}

let allAvailableNetworks: ComputedRef<INetwork[]> = computed(() => store.getAvailableNetworks);

function mkTblHdr_(o: { name: string, field?: string, classes?: string, align?: string, sortable?: boolean, headerClasses?: string, label: string, headerStyle?: string }) {
  return {
    ...o,
    field: o.field ?? o.name,
    classes: o.classes ?? "truncateWithElipsis",
    align: o.align ?? "left",
    sortable: o.sortable === undefined ? true : o.sortable,
    headerClasses: o.headerClasses ?? "truncateWithElipsis cellNoScribble"
  };
}
const headers = ref([
  mkTblHdr_({
    name: 'name',
    label: "Name",
    headerStyle: 'width: 20%; ',
  }),
  mkTblHdr_({
    name: 'CIDRs',
    label: "CIDRs",
    headerStyle: 'width: 10%; ',
  }),
  mkTblHdr_({
    name: 'active',
    label: "Active",
    align: "left",
    headerStyle: 'width: 10%; ',
  }),
  mkTblHdr_({
    name: 'status',
    label: "Status",
    headerStyle: 'width: 10%; ',
  }),
  mkTblHdr_({
    name: 'location',
    label: "Location",
    headerStyle: 'width: 20%; ',
  }),
  mkTblHdr_({
    name: 'internetInfo',
    label: "Internet",
    headerStyle: 'width: 20%; ',
  }),
  mkTblHdr_({
    name: 'devices',
    label: "Devices",
    headerStyle: 'width: 10%; ',
  }),
  mkTblHdr_({
    name: 'expand',
    label: "Details",
    align: 'center',
    headerStyle: 'width: 6.7em; ',
  }),
]);



const route = useRoute()
const router = useRouter()

let allDevices: ComputedRef<IDevice[]> = computed(() => store.getDevices);

let allNetworks: ComputedRef<INetwork[]> = computed(() => store.getAvailableNetworks);




let filtered: ComputedRef<boolean> = computed(() =>
  search.value != ""
);

function clearFilter() {
  selectedNetworkCurrent = undefined;
  selectedTimeframe = undefined;
  selectedServices = selectableServices.value.map((x) => x.value);
  search.value = "";
}


const loading = computed<boolean>(
  // could use numberOfOutstandingLoadRequests or numberOfTimesLoaded depending if we want to show when 'reloading'
  () => store.getLoading_Networks.numberOfOutstandingLoadRequests > 0
)


let filteredExtendedNetworks: ComputedRef<object[]> = computed(() => {
  const result: object[] = [];
  allNetworks.value.forEach((i) => {
    let lastSeenStr = moment(i.lastSeenAt).fromNow();
    let statusStr = "?";
    if (i.lastSeenAt != null && moment().diff(moment(i.lastSeenAt), "seconds") < 60) {
      lastSeenStr = "active";
      statusStr = "healthy"; // tmphack
    }
    const r: any = {
      ...i,
      name: GetNetworkName(i),
      CIDRs: GetNetworkCIDRs(i),
      active: lastSeenStr,
      internetInfo:
        (i.gateways == null ? "" : i.gateways.join(", ")) +
        (i.internetServiceProvider == null ? " " : " (" + i.internetServiceProvider.name + ")"),
      devices: GetDeviceIDsInNetwork(i, allDevices.value).length,
      status: statusStr,
      location: FormatLocation(i.geographicLocation),
    };
    if (
      search.value === "" ||
      JSON.stringify(r)
        .toLowerCase()
        .includes(search.value.toLowerCase())
    ) {
      // console.log("MATCH: this.search=", this.search, " and r=", JSON.stringify(r));
      result.push(r);
    } else {
      // console.log("NO MATCH: this.search=", this.search, " and r=", JSON.stringify(r));
    }
  });
  // use the default sort from API-Server for now...
  // result.sort((a: any, b: any) => {
  //   if (a.id < b.id) {
  //     return -1;
  //   }
  //   if (a.id > b.id) {
  //     return 1;
  //   }
  //   return 0;
  // });
  return result;
}
);

defineComponent({
  components: {
    ClearButton,
    ReadOnlyTextWithHover,
    Link2DetailsPage,
    NetworkDetails,
    Search,
    FilterSummaryMessage,
  },
});

// See https://github.com/storybookjs/storybook/issues/17954 for why we need this hack
var addHeaderSectionBugWorkaround = ref(false);

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

  addHeaderSectionBugWorkaround.value = true;
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
  <Teleport to="#CHILD_HEADER_SECTION" v-if="addHeaderSectionBugWorkaround">
    <q-toolbar class="justify-between secondary-toolbar">
      <Search v-model:searchFor="search" />
      <FilterSummaryMessage dense :filtered="filtered" :nItemsSelected="filteredExtendedNetworks.length"
        :nTotalItems="allNetworks?.length" itemsName="networks" />
      <ClearButton v-if="filtered" @click="clearFilter" />
    </q-toolbar>
    <q-toolbar class="justify-between secondary-toolbar">
      <q-select v-model="pageUserOptions.VisibleColumns" multiple dense options-dense
        :display-value="$q.lang.table.columns" emit-value map-options :options="headers" option-value="name"
        style="min-width: 150px" label="Shown" dark :options-dark="false" />
    </q-toolbar>
  </Teleport>
  <q-page padding class="justify-center row">
    <q-card class="pageCard listCard col-11">
      <q-card-section>
        <div class="row text-h5">
          Networks
        </div>
        <q-table table-class="itemList" :rows="filteredExtendedNetworks" :columns="headers" row-key="id" dense
          separator="none" :visible-columns="pageUserOptions.VisibleColumns" :pagination.sync="pagination" hide-bottom
          :loading="loading" flat>
          <template v-slot:body="props">
            <q-tr :props="props" @click="rowClicked(props)">
              <q-td :props="props" key="name">
                <ReadOnlyTextWithHover :message="props.row.name" />
              </q-td>
              <q-td :props="props" key="CIDRs">
                <ReadOnlyTextWithHover :message="props.row.CIDRs" />
              </q-td>
              <q-td :props="props" key="active">
                <ReadOnlyTextWithHover :message="props.row.active" />
              </q-td>
              <q-td :props="props" key="status">
                <ReadOnlyTextWithHover :message="props.row.status" />
              </q-td>
              <q-td :props="props" key="location">
                <ReadOnlyTextWithHover :message="props.row.location" />
              </q-td>
              <q-td :props="props" key="internetInfo">
                <ReadOnlyTextWithHover :message="props.row.internetInfo" />
              </q-td>
              <q-td :props="props" key="devices">
                <a :href="GetDevicesForNetworkLink(props.row.id)">{{ props.row.devices }}</a>
              </q-td>
              <q-td :props="props" key="manufacturerSummary">
                <ReadOnlyTextWithHover :message="props.row.manufacturerSummary" />
              </q-td>
              <q-td :props="props" key="expand">
                <div class="row no-wrap items-baseline">
                  <q-btn :icon="props.expand ? 'mdi-chevron-up' : 'mdi-chevron-down'" flat round dense
                    title="Toggle details expanded"></q-btn>
                  <Link2DetailsPage :link="'/#/network/' + props.row.id" />
                </div>
              </q-td>
            </q-tr>
            <q-tr v-if="props.expand" :props="props">
              <q-td :colspan="pageUserOptions.VisibleColumns.length">
                <NetworkDetails class="detailsSection z-top" :networkId="props.row.id" />
              </q-td>
            </q-tr>
          </template>
        </q-table>
      </q-card-section>
    </q-card>
  </q-page>

</template>


<style lang="scss" scoped>
// Based on .q-layout__section--marginal
.secondary-toolbar {
  background-color: var(--q-primary);
  color: #fff;
}

.detailsSection {
  margin-top: 1em;
  margin-left: 2em;
  margin-right: 1em;
  box-shadow: 4px 4px 8px 4px rgba(0, 0, 0, 0.2);
}

.listCard {
  margin-top: 10px;
  margin-left: 10px;
}

.cellNoScribble {
  text-overflow: ellipsis;
}

.itemList {
  margin-top: 10px;
}
</style>



<style lang="scss" >
.listCard table {
  table-layout: fixed;
}
</style>
