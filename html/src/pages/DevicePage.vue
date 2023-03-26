<script setup lang="ts">
import {
  watch,
  onMounted,
  onUnmounted,
  computed,
  ComputedRef,
  ref,
  Ref,
} from 'vue';
import { useRoute } from 'vue-router';

import { IDevice } from '../models/device/IDevice';
import { useQuasar } from 'quasar';
import { FormatSeenMap, FormatIDateTimeRange } from '../models/network/Utils';

// Components
import DeviceDetails from '../components/DeviceDetails.vue';

import { useNetStateStore } from '../stores/Net-State-store';

const $q = useQuasar();
const store = useNetStateStore();

const props = defineProps({
  selectedNetworkink: { type: String, required: false, default: null },
});

let polling: undefined | NodeJS.Timeout;

const emit = defineEmits(['update:breadcrumbs', 'update:contextMenu']);

const kRefreshFrequencyInSeconds_: number = 15;

let deviceDetailsComponent: any = null;

onMounted(() => {
  // first time check quickly, then more gradually
  store.fetchDevice(route.params.id as string);
  store.fetchAvailableNetworks();
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    store.fetchDevice(route.params.id as string);
    store.fetchAvailableNetworks();
  }, kRefreshFrequencyInSeconds_ * 1000);

  addHeaderSectionBugWorkaround.value = true;
});

onUnmounted(() => {
  clearInterval(polling);
});

const route = useRoute();

let device: ComputedRef<IDevice | null> = computed(() => {
  return store.getDevice(route.params.id as string);
});
let deviceSeen: ComputedRef<object | undefined> = computed(() => {
  if (device.value?.seen) {
    return device.value?.seen['Ever'];
  }
  return undefined;
});

function doReScan() {
  deviceDetails.value?.rescanDevice();
}
function doEditName() {
  deviceDetails.value?.editName();
}
function doEditNotes() {
  deviceDetails.value?.editNotes();
}

watch(
  () => device.value,
  async (device) => {
    // @todo CODE sharing with predefined routes
    // console.log(`entering wtch device=${device}, device.aggregatedBy=${device?.aggregatedBy}`)
    if (device) {
      if (device.aggregatedBy) {
        emit('update:breadcrumbs', [
          { text: 'Home', href: '/#/' },
          { text: 'Devices', href: '/#/devices' },
          {
            text: device.names[0].name,
            href: '/#/device/' + device.aggregatedBy,
          },
          {
            text: FormatIDateTimeRange(deviceSeen.value, true),
            disabled: true,
          },
        ]);
      } else {
        emit('update:breadcrumbs', [
          { text: 'Home', href: '/#/' },
          { text: 'Devices', href: '/#/devices' },
          { text: device.names[0].name, disabled: true },
        ]);
        emit('update:contextMenu', [
          { name: 'Edit Device Name', enabled: true, onClick: doEditName },
          { name: 'Edit Device Notes', enabled: true, onClick: doEditNotes, dividerAfter:true },
          { name: 'Re-Scan this device', enabled: true, onClick: doReScan },
        ]);
      }
    }
  },
  { immediate: true }
);

const deviceDetails: Ref<any> = ref(null);

// See https://github.com/storybookjs/storybook/issues/17954 for why we need this hack
var addHeaderSectionBugWorkaround = ref(false);

var showOldNetworks = ref(false);
var showInactiveInterfaces = ref(false);
var showSeenDetails = ref(false);
</script>

<template>
  <Teleport to="#CHILD_HEADER_SECTION" v-if="addHeaderSectionBugWorkaround">
    <q-toolbar class="justify-between secondary-toolbar">
      <q-checkbox dense v-model="showOldNetworks" label="Show Old Networks" />
      <q-checkbox
        dense
        v-model="showInactiveInterfaces"
        label="Show Inactive Network Interfaces"
      />
      <q-checkbox dense v-model="showSeenDetails" label="Show Seen Details" />
    </q-toolbar>
  </Teleport>
  <q-page padding class="justify-center row">
    <q-card class="pageCard col-11">
      <q-card-section class="text-subtitle2" style="padding-bottom: 0">
        Device Details for
        {{ device == null ? 'loading...' : '"' + device.name + '"' }}
        <span class="snapshot" v-if="device?.aggregatedBy">{snapshot}</span>
      </q-card-section>
      <q-card-section>
        <DeviceDetails
          ref="deviceDetails"
          v-if="device"
          :device="device"
          :showExtraDetails="true"
          :allowEdit="true"
          :showOldNetworks="showOldNetworks"
          :showInactiveInterfaces="showInactiveInterfaces"
          :showSeenDetails="showSeenDetails"
        />
      </q-card-section>
    </q-card>
  </q-page>
</template>
