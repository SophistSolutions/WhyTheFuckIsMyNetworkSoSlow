<script setup lang="ts">
import { onMounted, onUnmounted, computed } from "vue";

import JsonViewer from "vue-json-viewer";

import { INetworkInterface } from "../models/network/INetworkInterface";

import {
  FormatIDateTimeRange,FormatBaudRate
} from "../models/network/Utils";

import { PluralizeNoun } from "src/utils/Linguistics";

import ReadOnlyTextWithHover from "../components/ReadOnlyTextWithHover.vue";

import { useNetStateStore } from "../stores/Net-State-store";

const store = useNetStateStore();

interface Props {
  // EITHER networkInterface or networkInterfaceId - EXCLUSIVE and ONE required
  networkInterface?: INetworkInterface;
  networkInterfaceId?: string;
  includeLinkToDetailsPage?: boolean;
  showExtraDetails?: boolean;
  showInactiveInterfaces?: boolean;
}
const props = withDefaults(defineProps<Props>(), {
  includeLinkToDetailsPage: false,
  showExtraDetails: false,
  showInactiveInterfaces: true,
});

let polling: undefined | NodeJS.Timeout;

let currentNetworkInterface = computed<INetworkInterface | undefined>(
  () =>
    props.networkInterface ??
    store.getNetworkInterface(props.networkInterfaceId as string)
);

function fetchStuffForInterfaceIDSince(netInterfaceIDs: string[]) {
  netInterfaceIDs.forEach((netInterfaceID) => {
    store.fetchNetworkInterfaces([netInterfaceID]);
    const thatNetInterface = store.getNetworkInterface(netInterfaceID);
    if (thatNetInterface && thatNetInterface.attachedToDevices) {
      store.fetchDevices(thatNetInterface.attachedToDevices);
    }
  });
}

function doFetches() {
  if (props.networkInterfaceId) {
    store.fetchNetworkInterfaces([props.networkInterfaceId]);
  }
  if (currentNetworkInterface && currentNetworkInterface.value?.aggregatesReversibly) {
    fetchStuffForInterfaceIDSince(currentNetworkInterface.value?.aggregatesReversibly);
  }
  if (currentNetworkInterface && currentNetworkInterface.value?.aggregatesIrreversibly) {
    fetchStuffForInterfaceIDSince(currentNetworkInterface.value?.aggregatesIrreversibly);
  }
}

onMounted(() => {
  // first time check immediately, then more gradually for updates
  doFetches();
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    doFetches();
  }, 30 * 1000);
});

onUnmounted(() => {
  clearInterval(polling);
});

function getSeenForNetworkInterface(netIFace?: INetworkInterface) {
  if (netIFace && netIFace.attachedToDevices) {
    const deviceIDWithInterface = netIFace.attachedToDevices.find((deviceID: string) =>
      store.getDevice(deviceID)
    );
    if (deviceIDWithInterface) {
      const d = store.getDevice(deviceIDWithInterface);
      if (d && d.seen) {
        return d.seen["Ever"];
      }
    }
  }
  return undefined;
}
function getSeenForNetworkInterfaceID(netInterfaceID: string) {
  return getSeenForNetworkInterface(store.getNetworkInterface(netInterfaceID));
}
function isShown()
{
  if (props.showInactiveInterfaces) {
    return true;
  }
  if (!currentNetworkInterface.value) {
    return false; // don't show if nothing loaded yet
  }
  if (!currentNetworkInterface.value.status) {
    return true; // harder - really in this case - if we are a rollup, want to check if any rolled up items active, but do this for now.
  }
  return currentNetworkInterface.value.status.includes ("Running");
}
</script>

<template>
  <!--
    @todo add a page with networkInnterface as only argument, and then have LINK to one of those and make links
    in 'aggreates' below point ot that page so you can easily link/view one of the detail interfaces... (now you have to
    follow the dated network or device itself to find corresponding link)
  -->
  <div v-if="currentNetworkInterface && isShown ()" class="q-pa-sm">
    <div class="row" v-if="currentNetworkInterface?.friendlyName">
      <div class="col-3">Friendly Name</div>
      <div class="col">
        {{ currentNetworkInterface.friendlyName }}
        <span class="snapshot" v-if="currentNetworkInterface.aggregatedBy"
          >{snapshot}</span
        >
      </div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.type">
      <div class="col-3">Type</div>
      <div class="col">{{ currentNetworkInterface.type }}</div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.description">
      <div class="col-3">Description</div>
      <div class="col">{{ currentNetworkInterface.description }}</div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.platformInterfaceID">
      <div class="col-3">Platform Interface ID</div>
      <div class="col">{{ currentNetworkInterface.platformInterfaceID }}</div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.status">
      <div class="col-3">Status</div>
      <div class="col">{{ currentNetworkInterface.status.join(", ") }}</div>
    </div>
    <div class="row">
      <div class="col-3">ID</div>
      <div class="col">
        <ReadOnlyTextWithHover
          :message="((props.networkInterfaceId || currentNetworkInterface?.id) as string)"
          :link="
            '/#/network-interface/' +
            (props.networkInterfaceId || currentNetworkInterface?.id)
          "
        />
      </div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.hardwareAddress">
      <div class="col-3">Hardware Address</div>
      <div class="col">{{ currentNetworkInterface.hardwareAddress }}</div>
    </div>
    <div class="row" v-if="currentNetworkInterface">
      <div class="col-3">Seen</div>
      <div class="col">{{ FormatIDateTimeRange(getSeenForNetworkInterface(currentNetworkInterface)) }}</div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.boundAddressRanges">
      <div class="col-3">CIDRs</div>
      <div class="col">{{ currentNetworkInterface.boundAddressRanges.join(", ") }}</div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.boundAddresses">
      <div class="col-3">Bindings</div>
      <div class="col">{{ currentNetworkInterface.boundAddresses.join(", ") }}</div>
    </div>
    <div
      class="row"
      v-if="
        currentNetworkInterface?.transmitSpeedBaud ||
        currentNetworkInterface?.receiveLinkSpeedBaud
      "
    >
      <div class="col-3">Speed (tx / rx)</div>
      <div class="col">
        {{ FormatBaudRate(currentNetworkInterface.transmitSpeedBaud) }} / {{
          FormatBaudRate(currentNetworkInterface.receiveLinkSpeedBaud)
        }}
      </div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.DNSServers">
      <div class="col-3">DNS Servers</div>
      <div class="col">{{ currentNetworkInterface.DNSServers.join(", ") }}</div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.gateways">
      <div class="col-3">Gateways</div>
      <div class="col">{{ currentNetworkInterface.gateways.join(", ") }}</div>
    </div>
    <div
      class="row"
      v-if="props.showExtraDetails && currentNetworkInterface?.attachedToDevices"
    >
      <div class="col-3">Attached to Device(s)</div>
      <div class="col">
        <span
          v-for="d in currentNetworkInterface.attachedToDevices"
          v-bind:key="d"
          class="aggregatesItem"
        >
          <ReadOnlyTextWithHover :message="d" :link="'/#/device/' + d" />;&nbsp;
        </span>
      </div>
    </div>
    <div
      class="row"
      v-if="
        currentNetworkInterface?.aggregatesReversibly &&
        currentNetworkInterface.aggregatesReversibly.length &&
        props.showExtraDetails
      "
    >
      <div class="col-3">Aggregates Reversibly</div>
      <div class="col">
        <div class="row wrap">
          <span
            v-for="aggregateNetInterfaceID in currentNetworkInterface.aggregatesReversibly"
            v-bind:key="aggregateNetInterfaceID"
            class="aggregatesItem"
          >
            <ReadOnlyTextWithHover
              :message="
                FormatIDateTimeRange(
                  getSeenForNetworkInterfaceID(aggregateNetInterfaceID),
                  true
                ) ?? aggregateNetInterfaceID
              "
              :link="'/#/network-interface/' + aggregateNetInterfaceID"
            />;&nbsp;
          </span>
        </div>
      </div>
    </div>
    <div
      class="row"
      v-if="
        currentNetworkInterface?.aggregatesIrreversibly &&
        currentNetworkInterface.aggregatesIrreversibly.length &&
        props.showExtraDetails
      "
    >
      <div class="col-3">Aggregates Irreversibly</div>
      <div class="col">
        <div class="row wrap">
          <span
            v-for="aggregateNetInterfaceID in currentNetworkInterface.aggregatesIrreversibly"
            v-bind:key="aggregateNetInterfaceID"
            class="aggregatesItem"
          >
            <ReadOnlyTextWithHover
              :message="
                FormatIDateTimeRange(
                  getSeenForNetworkInterfaceID(aggregateNetInterfaceID),
                  true
                ) ?? aggregateNetInterfaceID
              "
            />;&nbsp;
          </span>
        </div>
      </div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.wirelessInformation">
      <div class="col-3">Wireless</div>
      <div class="col">
        <div class="row wrap">
          <div class="col-4">SSID</div>
          <div>{{ currentNetworkInterface?.wirelessInformation.SSID }}</div>
        </div>
        <div
          class="row wrap"
          v-if="currentNetworkInterface?.wirelessInformation.profileName"
        >
          <div class="col-4">profileName</div>
          <div>{{ currentNetworkInterface?.wirelessInformation.profileName }}</div>
        </div>
        <div class="row wrap">
          <div class="col-4">State</div>
          <div>{{ currentNetworkInterface?.wirelessInformation.state }}</div>
        </div>
        <div
          class="row wrap"
          v-if="currentNetworkInterface?.wirelessInformation.signalQuality"
        >
          <div class="col-4">Signal Quality</div>
          <div>{{ currentNetworkInterface?.wirelessInformation.signalQuality }}</div>
        </div>
        <div
          class="row wrap"
          v-if="currentNetworkInterface?.wirelessInformation.MACAddress"
        >
          <div class="col-4">MACAddress</div>
          <div>{{ currentNetworkInterface?.wirelessInformation.MACAddress }}</div>
        </div>
        <div
          class="row wrap"
          v-if="currentNetworkInterface?.wirelessInformation.authAlgorithm"
        >
          <div class="col-4">Auth Algorithm</div>
          <div>{{ currentNetworkInterface?.wirelessInformation.authAlgorithm }}</div>
        </div>
        <div class="row wrap" v-if="currentNetworkInterface?.wirelessInformation.cipher">
          <div class="col-4">Cipher</div>
          <div>{{ currentNetworkInterface?.wirelessInformation.cipher }}</div>
        </div>
        <div class="row wrap" v-if="currentNetworkInterface?.wirelessInformation.BSSType">
          <div class="col-4">BSSType</div>
          <div>{{ currentNetworkInterface?.wirelessInformation.BSSType }}</div>
        </div>
        <div
          class="row wrap"
          v-if="currentNetworkInterface?.wirelessInformation.physicalConnectionType"
        >
          <div class="col-4">Physical Connection Type</div>
          <div>
            {{ currentNetworkInterface?.wirelessInformation.physicalConnectionType }}
          </div>
        </div>
        <div
          class="row wrap"
          v-if="currentNetworkInterface?.wirelessInformation.securityEnabled"
        >
          <div class="col-4">Security</div>
          <div>
            {{
              currentNetworkInterface?.wirelessInformation.securityEnabled == true
                ? "enabled"
                : "disabled"
            }}
          </div>
        </div>
        <div
          class="row wrap"
          v-if="currentNetworkInterface?.wirelessInformation.x8021Enabled"
        >
          <div class="col-4">8021x Enabled</div>
          <div>{{ currentNetworkInterface?.wirelessInformation.x8021Enabled }}</div>
        </div>
        <div
          class="row wrap"
          v-if="currentNetworkInterface?.wirelessInformation.connectionMode"
        >
          <div class="col-4">Connection Mode</div>
          <div>{{ currentNetworkInterface?.wirelessInformation.connectionMode }}</div>
        </div>
      </div>
    </div>
    <div class="row" v-if="currentNetworkInterface?.debugProps && props.showExtraDetails">
      <div class="col-3">DEBUG INFO</div>
      <div class="col">
        <json-viewer
          :value="currentNetworkInterface.debugProps"
          :expand-depth="0"
          copyable
          sort
          class="debugInfoJSONViewers"
        />
      </div>
    </div>
  </div>
</template>

<style scoped lang="scss">
.snapshot {
  font-style: italic;
}

.aggregatesItem {
  min-width: 10em;
}
</style>
