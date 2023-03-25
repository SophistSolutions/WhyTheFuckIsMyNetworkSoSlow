<script setup lang="ts">
import { onMounted, onUnmounted, computed, ComputedRef } from "vue";
import { useRoute } from "vue-router";
import { useQuasar } from "quasar";
import { watch } from "vue";
import { IDevice } from "../models/device/IDevice";
import { INetworkInterface } from "../models/network/INetworkInterface";

import NetworkInterfaceDetails from "../components/NetworkInterfaceDetails.vue";

import { FormatIDateTimeRange } from "../models/network/Utils";
import { useNetStateStore } from "../stores/Net-State-store";

const $q = useQuasar();
const store = useNetStateStore();

let polling: undefined | NodeJS.Timeout;
const route = useRoute();

const emit = defineEmits(["update:breadcrumbs"]);

onMounted(() => {
  // first time check quickly, then more gradually
  doFetches_();
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    doFetches_();
  }, 15 * 1000);
});

onUnmounted(() => {
  clearInterval(polling);
});

let networkInterface: ComputedRef<INetworkInterface | undefined> = computed(() => {
  return store.getNetworkInterface(route.params.id as string);
});
let owningDeviceID: ComputedRef<string | undefined> = computed(() => {
  if (networkInterface.value && networkInterface.value.attachedToDevices) {
    return networkInterface.value.attachedToDevices[0]; // @todo consider how to handle if more than one - zero sb undefined
  }
  return undefined;
});
let owningDevice: ComputedRef<IDevice | undefined> = computed(() => {
  if (owningDeviceID.value) {
    return store.getDevice(owningDeviceID.value);
  }
  return undefined;
});
let networkInterfaceSeen: ComputedRef<object | undefined> = computed(() => {
  if (owningDevice.value?.seen) {
    return owningDevice.value?.seen["Ever"];
  }
  return undefined;
});

function doFetches_() {
  store.fetchNetworkInterfaces([route.params.id as string]);
  if (owningDeviceID.value) {
    store.fetchDevice(owningDeviceID.value);
  }
}

watch(
  () => networkInterface.value,
  async (networkInterface) => {
    if (owningDevice.value == null) {
      doFetches_(); // first time didn't know device to fetch, so refetch...
    }
    if (networkInterface) {
      if (networkInterface.aggregatedBy) {
        emit("update:breadcrumbs", [
          { text: "Home", href: "/#/" },
          { text: "Network Interfaces" },
          {
            text: networkInterface.friendlyName,
            href: "/#/network-interface/" + networkInterface.aggregatedBy,
          },
          {
            text: FormatIDateTimeRange(networkInterfaceSeen.value, true),
            disabled: true,
          },
        ]);
      } else {
        emit("update:breadcrumbs", [
          { text: "Home", href: "/#/" },
          { text: "Network Interfaces" },
          { text: networkInterface.friendlyName, disabled: true },
        ]);
      }
    }
  } ,{immediate:true}
);
</script>

<template>
  <q-page padding class="justify-center row">
    <q-card class="pageCard col-11">
      <q-card-section class="text-subtitle2" style="padding-bottom: 0">
        Network Interface
        {{
          networkInterface == null
            ? "loading..."
            : '"' + networkInterface.friendlyName + '"'
        }}
        <span class="snapshot" v-if="networkInterface?.aggregatedBy">{snapshot}</span>
      </q-card-section>
      <q-card-section style="margin-top: 0">
        <NetworkInterfaceDetails
          :networkInterface="networkInterface"
          v-if="networkInterface"
          :showExtraDetails="true"
        />
      </q-card-section>
    </q-card>
  </q-page>
</template>
