<script setup lang="ts">
import { onMounted, onUnmounted, Ref, ref, computed, ComputedRef } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import moment from 'moment';
import { useQuasar } from 'quasar';
import { useStorage } from '@vueuse/core';

import { IDevice } from '../models/device/IDevice';
import {
  ComputeDeviceTypeIconURLs,
  ComputeOSIconURLList,
  ComputeServiceTypeIconURLs,
} from '../models/device/Utils';
import { INetwork } from '../models/network/INetwork';
import {
  FormatAttachedNetworkLocalAddresses,
  GetNetworkLink,
  GetNetworkName,
  GetServices,
  SortNetworks,
  GetAttachedNetworksAsNetworks,
} from '../models/network/Utils';

// Components
import Search from '../components/Search.vue';
import ClearButton from '../components/ClearButton.vue';
import DeviceDetails from '../components/DeviceDetails.vue';
import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';
import Link2DetailsPage from '../components/Link2DetailsPage.vue';
import FilterSummaryMessage from '../components/FilterSummaryMessage.vue';

import { useNetStateStore } from '../stores/Net-State-store';
const $q = useQuasar();

const store = useNetStateStore();

const props = defineProps({
  selectedNetworkink: { type: String, required: false, default: null },
});

let polling: null | NodeJS.Timeout = null;
var search = ref('');

const loading = computed<boolean>(
  // could use numberOfOutstandingLoadRequests or numberOfTimesLoaded depending if we want to show when 'reloading'
  () => store.getLoading_Devices.numberOfOutstandingLoadRequests > 0
);

const selectableServices = ref([
  {
    label: 'Other',
    value: 'other',
  },
  {
    label: 'Print',
    value: 'print',
  },
  {
    label: 'RDP (Remote Desktop)',
    value: 'rdp',
  },
  {
    label: 'SSH',
    value: 'ssh',
  },
  {
    label: 'SMB (Windows Network FS)',
    value: 'smb',
  },
  {
    label: 'Web (HTTP/HTTPS)',
    value: 'web',
  },
]);

let selectedServices = ref(selectableServices.value.map((x) => x.value));

const filterIsSetToAllowAllServices = computed<boolean>(() => {
  return selectedServices.value.length === selectableServices.value.length;
});

const filterIsSetToAllowEmptyServices = computed<boolean>(() => {
  return selectedServices.value.length === 0;
});

const selectServicesFilter_icon = computed<string>(() => {
  if (filterIsSetToAllowAllServices.value) {
    return 'mdi-close-box';
  }
  if (
    selectedServices.value.length > 0 &&
    !filterIsSetToAllowAllServices.value
  ) {
    return 'mdi-minus-box';
  }
  return 'mdi-checkbox-blank-outline';
});

function selectServicesFilter_ToggleSelectAll() {
  if (filterIsSetToAllowAllServices.value) {
    selectedServices.value = [];
  } else {
    selectedServices.value = selectableServices.value.map((x) => x.value);
  }
}

const selectableNetworks = computed<object[]>(() => {
  const r: object[] = [
    {
      label: 'Any',
      value: null,
    },
  ];
  SortNetworks(allAvailableNetworks.value).forEach((n) => {
    r.push({
      label: GetNetworkName(n),
      value: n.id,
    });
  });
  return r;
});
let selectedNetworkCurrent: Ref<string | null> = ref(null);

const selectableTimeframes = ref<object[]>([
  {
    label: 'Ever',
    value: null,
  },
  {
    label: 'Last Few Minutes',
    value: 'PT3.9M',
  },
  {
    label: 'Last Hour',
    value: 'PT1H',
  },
  {
    label: 'Last Day',
    value: 'P1D',
  },
  {
    label: '>15 Min Ago',
    value: '-PT15M',
  },
  {
    label: '>1 Day Ago',
    value: '-P1D',
  },
]);
let selectedTimeframe: Ref<string | null> = ref(null);

function rowClicked(props: object) {
  props.expand = !props.expand;
}

let allAvailableNetworks: ComputedRef<INetwork[]> = computed(
  () => store.getAvailableNetworks
);

function mkTblHdr_(o: {
  name: string;
  classes?: string;
  align?: string;
  sortable?: boolean;
  headerClasses?: string;
  label: string;
  headerStyle?: string;
  sort?: any;
  field?: any;
}) {
  return {
    ...o,
    field: o.field ?? o.name,
    classes: o.classes ?? 'truncateWithElipsis',
    align: o.align ?? 'left',
    sortable: o.sortable === undefined ? true : o.sortable,
    headerClasses: o.headerClasses ?? 'truncateWithElipsis cellNoScribble',
  };
}
const tableHeaders = ref([
  mkTblHdr_({
    name: 'name',
    label: 'Name',
    headerStyle: 'width: 20%; ',
  }),
  mkTblHdr_({
    name: 'type',
    label: 'Type',
    headerStyle: 'width: 5em',
  }),
  mkTblHdr_({
    name: 'lastSeenAt',
    label: 'Last Seen',
    classes: 'truncateWithElipsis cellNoScribble',
    headerStyle: 'width: 9em; ',
    field: (row) => row.seen?.Ever?.upperBound,
  }),
  mkTblHdr_({
    name: 'manufacturerSummary',
    label: 'Manufacturer',
    classes: 'truncateWithElipsis cellNoScribble',
    headerStyle: 'width: 18%; ',
  }),
  mkTblHdr_({
    name: 'operatingSystem',
    label: 'OS',
    headerStyle: 'width: 5em',
  }),
  mkTblHdr_({
    name: 'services',
    label: 'Services',
    align: 'center',
    headerStyle: 'width: 7em',
  }),
  mkTblHdr_({
    name: 'localAddresses',
    label: 'Local Address',
    headerStyle: 'width: 12%; ',
  }),
  mkTblHdr_({
    name: 'networksSummary',
    field: 'attachedNetworks',
    label: 'Networks',
    headerStyle: 'width: 12%; ',
  }),
  mkTblHdr_({
    name: 'expand',
    label: 'Details',
    align: 'center',
    headerStyle: 'width: 6.7em; ',
  }),
]);

// store page options in local storage, but eventually add many more options here (like collapsed or open show filter section etc)
// COULD POSSIBLY also store stuff like selected filter (search string etc) - but no need for now....
const pageUserOptions = useStorage('Devices-Page-User-Options', {
  VisibleColumns: [
    'name',
    'type',
    'lastSeenAt',
    'manufacturerSummary',
    'operatingSystem',
    'services',
    'localAddresses',
    'networksSummary',
    'expand',
  ],
});

const route = useRoute();
const router = useRouter();

let allDevices: ComputedRef<IDevice[]> = computed(() => store.getDevices);

let filtered: ComputedRef<boolean> = computed(
  () =>
    selectedNetworkCurrent.value != null ||
    selectedTimeframe.value !== null ||
    search.value != '' ||
    !filterIsSetToAllowAllServices.value
);

function clearFilter() {
  selectedNetworkCurrent.value = null;
  selectedTimeframe.value = null;
  selectedServices.value = selectableServices.value.map((x) => x.value);
  search.value = '';
}

let filteredExtendedDevices: ComputedRef<object[]> = computed(() => {
  const result: object[] = [];
  allDevices.value.forEach((i: IDevice) => {
    let passedFilter = i != null;
    if (
      passedFilter &&
      !(
        selectedNetworkCurrent.value == null ||
        Object.prototype.hasOwnProperty.call(
          i.attachedNetworks,
          selectedNetworkCurrent.value
        )
      )
    ) {
      passedFilter = false;
    }
    if (passedFilter && selectedTimeframe.value !== null) {
      if (i.seen?.Ever?.upperBound == null) {
        // not sure how to treat missing data so for now treat as passed filter
      } else {
        const timeSinceSeenAsMS =
          Date.now() - new Date(i.seen?.Ever?.upperBound).getTime();
        const selectedTimeframeAsMS = moment
          .duration(selectedTimeframe.value)
          .asMilliseconds();
        // selectedTimeframeAsMS can be > 0 or < 0
        if (selectedTimeframeAsMS > 0) {
          if (timeSinceSeenAsMS > selectedTimeframeAsMS) {
            passedFilter = false;
          }
        } else {
          if (timeSinceSeenAsMS < -selectedTimeframeAsMS) {
            passedFilter = false;
          }
        }
      }
    }
    if (passedFilter && !filterIsSetToAllowAllServices.value) {
      var itsServices = GetServices(i);
      if (filterIsSetToAllowEmptyServices.value) {
        passedFilter = itsServices.length == 0;
      } else {
        passedFilter =
          itsServices
            .map((x) => x.name)
            .filter((value) => selectedServices.value.includes(value)).length >
          0;
      }
    }
    if (passedFilter) {
      const r = {
        ...i,
        ...{
          localAddresses: FormatAttachedNetworkLocalAddresses(
            i.attachedNetworks
          ),
          manufacturerSummary:
            i.manufacturer == null
              ? ''
              : i.manufacturer.fullName || i.manufacturer.shortName,
          services: GetServices(i),
        },
      };
      if (
        search.value === '' ||
        JSON.stringify(r).toLowerCase().includes(search.value.toLowerCase())
      ) {
        result.push(r);
      }
    }
  });
  // standardize results to same order each time, and let list control do real user sorting
  // NO- DEFAULT TO ORDER RETURNED BY WEBSERVICE, SINCE THAT MAYBE THE MOST NATURAL ORDER
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
});

const kRefreshFrequencyInSeconds_: number = 15;

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
    selectedNetworkCurrent.value = s;
  }

  // first time check quickly, then more gradually
  store.fetchActiveDevices();
  store.fetchAvailableNetworks();
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    store.fetchActiveDevices();
    store.fetchAvailableNetworks();
  }, kRefreshFrequencyInSeconds_ * 1000);

  addHeaderSectionBugWorkaround.value = true;
});

onUnmounted(() => {
  if (polling) {
    clearInterval(polling);
  }
});

// See https://github.com/storybookjs/storybook/issues/17954 for why we need this hack
var addHeaderSectionBugWorkaround = ref(false);

// disable pagination
const pagination = ref({
  page: 1,
  rowsPerPage: 0,
});
</script>

<template>
  <Teleport to="#CHILD_HEADER_SECTION" v-if="addHeaderSectionBugWorkaround">
    <q-toolbar class="justify-between secondary-toolbar">
      <q-select
        dense
        hide-details="true"
        :options="selectableNetworks"
        v-model="selectedNetworkCurrent"
        emit-value
        map-options
        label="On network"
        style="min-width: 150px"
        dark
        :options-dark="false"
      />
      <q-select
        dense
        hide-details="true"
        :options="selectableTimeframes"
        v-model="selectedTimeframe"
        label="Seen"
        emit-value
        map-options
        style="min-width: 150px"
        dark
        :options-dark="false"
      />
      <q-select
        dense
        small
        multiple
        hide-details="true"
        hint="Any=>no filter; multiple=>OR"
        hide-hint
        :options="selectableServices"
        emit-value
        map-options
        v-model="selectedServices"
        label="With services"
        style="min-width: 150px"
        dark
        :options-dark="false"
      >
        <template v-slot:before-options>
          <q-item>
            <q-item-section @click="selectServicesFilter_ToggleSelectAll">
              <div class="row no-wrap items-baseline">
                <q-icon
                  :color="selectedServices.length > 0 ? 'indigo darken-4' : ''"
                  :name="selectServicesFilter_icon"
                  left
                />
                {{
                  filterIsSetToAllowAllServices ? 'Select None' : 'Select All'
                }}
              </div>
            </q-item-section>
          </q-item>
          <q-separator />
        </template>
        <template v-slot:selected>
          <div v-if="selectableServices.length == selectedServices.length">
            Any
          </div>
          <q-chip dark dense v-if="1 == selectedServices.length">
            {{
              selectableServices.find((a) => a.value == selectedServices[0])
                ?.label
            }}
          </q-chip>
          <q-chip
            dark
            dense
            v-if="1 < selectedServices.length && !filterIsSetToAllowAllServices"
          >
            {{
              selectableServices.find((a) => a.value == selectedServices[0])
                ?.label
            }}
          </q-chip>
          <q-chip
            dark
            dense
            v-if="1 < selectedServices.length && !filterIsSetToAllowAllServices"
          >
            + {{ selectedServices.length - 1 }} other(s)
          </q-chip>
          <div v-if="filterIsSetToAllowEmptyServices">None</div>
        </template>
      </q-select>
      <Search v-model:searchFor="search" />
      <FilterSummaryMessage
        dense
        :filtered="filtered"
        :nItemsSelected="filteredExtendedDevices.length"
        :nTotalItems="allDevices?.length"
        itemsName="devices"
      />
      <ClearButton v-if="filtered" @click="clearFilter" />
    </q-toolbar>
    <q-toolbar class="justify-between secondary-toolbar">
      <q-select
        v-model="pageUserOptions.VisibleColumns"
        multiple
        dense
        options-dense
        :display-value="$q.lang.table.columns"
        emit-value
        map-options
        :options="tableHeaders"
        option-value="name"
        style="min-width: 150px"
        label="Shown"
        dark
        :options-dark="false"
      />
    </q-toolbar>
  </Teleport>
  <q-page padding class="justify-center row">
    <q-card class="pageCard listCard col-11">
      <q-card-section style="border: none">
        <div class="row text-h5">Devices</div>
        <q-table
          dense
          table-class="itemList "
          :rows="filteredExtendedDevices"
          :columns="tableHeaders"
          separator="none"
          row-key="id"
          :visible-columns="pageUserOptions.VisibleColumns"
          :pagination.sync="pagination"
          hide-bottom
          flat
          :loading="loading"
        >
          <template v-slot:body="props">
            <q-tr :props="props">
              <q-td :props="props" key="name">
                <ReadOnlyTextWithHover
                  :message="props.row.name"
                  :link="'/#/device/' + props.row.id"
                />
              </q-td>
              <q-td :props="props" key="type">
                <span
                  v-for="(t, i) in ComputeDeviceTypeIconURLs(props.row.type)"
                  :key="i"
                >
                  <img
                    v-if="t.url"
                    :src="t.url"
                    :title="t.label"
                    height="20"
                    width="20"
                  />
                  <ReadOnlyTextWithHover v-if="!t.url" :message="t.label" />
                </span>
              </q-td>
              <q-td :props="props" key="lastSeenAt">
                <ReadOnlyTextWithHover
                  v-if="props.row.seen?.Ever?.upperBound"
                  :message="moment(props.row.seen.Ever.upperBound).fromNow()"
                />
              </q-td>
              <q-td :props="props" key="manufacturerSummary">
                <ReadOnlyTextWithHover
                  :message="props.row.manufacturerSummary"
                />
              </q-td>
              <q-td :props="props" key="operatingSystem">
                <span
                  v-for="(t, i) in ComputeOSIconURLList(
                    props.row.operatingSystem
                  )"
                  :key="i"
                >
                  <img
                    v-if="t.url"
                    :src="t.url"
                    :title="t.label"
                    height="20"
                    width="20"
                  />
                  <ReadOnlyTextWithHover v-if="!t.url" :message="t.label" />
                </span>
              </q-td>
              <q-td :props="props" key="services">
                <span v-for="(s, i) in props.row.services" :key="i">
                  <span
                    v-for="(t, i) in ComputeServiceTypeIconURLs([s.name])"
                    :key="i"
                  >
                    <img
                      v-if="t.url"
                      :src="t.url"
                      :title="t.label"
                      height="20"
                      width="20"
                    />
                    <ReadOnlyTextWithHover v-if="!t.url" :message="t.label" />
                  </span>
                </span>
              </q-td>
              <q-td :props="props" key="localAddresses">
                <ReadOnlyTextWithHover :message="props.row.localAddresses" />
              </q-td>
              <q-td :props="props" key="networksSummary">
                <span
                  v-for="anw in GetAttachedNetworksAsNetworks(
                    props.row.attachedNetworks,
                    allAvailableNetworks
                  )"
                  :key="anw.id"
                >
                  <ReadOnlyTextWithHover
                    :message="GetNetworkName(anw)"
                    :link="GetNetworkLink(anw)"
                  />&nbsp;
                </span>
              </q-td>
              <q-td :props="props" key="expand">
                <div class="row no-wrap items-baseline">
                  <q-btn
                    @click="rowClicked(props)"
                    :icon="props.expand ? 'mdi-chevron-up' : 'mdi-chevron-down'"
                    flat
                    round
                    dense
                    title="Toggle details expanded"
                  ></q-btn>
                  <Link2DetailsPage :link="'/#/device/' + props.row.id" />
                </div>
              </q-td>
            </q-tr>
            <q-tr v-if="props.expand" :props="props">
              <q-td :colspan="pageUserOptions.VisibleColumns.length">
                <DeviceDetails
                  class="detailsSection z-top"
                  :deviceId="props.row.id"
                  :includeLinkToDetailsPage="true"
                />
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
  margin-top: -6pt;
  margin-left: -6pt;
  margin-right: -6pt;
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

<style lang="scss">
.listCard table {
  table-layout: fixed;
}
</style>
