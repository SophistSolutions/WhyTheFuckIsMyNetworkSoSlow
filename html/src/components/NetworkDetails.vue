<script setup lang="ts">
import {
  onMounted,
  onUnmounted,
  computed,
  ComputedRef,
} from 'vue';

import JsonViewer from 'vue-json-viewer';
import moment from 'moment';

import { IDevice } from '../models/device/IDevice';
import { INetworkInterface } from '../models/network/INetworkInterface';

import {
  FormatLocation,
  GetDeviceIDsInNetwork,
  GetDevicesForNetworkLink,
  GetNetworkCIDRs,
  GetNetworkName,
  FormatIDateTimeRange,
} from '../models/network/Utils';

import { PluralizeNoun } from 'src/utils/Linguistics';

// Components
import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';
import Link2DetailsPage from '../components/Link2DetailsPage.vue';
import NetworkInterfacesDetails from '../components/NetworkInterfacesDetails.vue';

import { useNetStateStore } from '../stores/Net-State-store';
import { INetwork } from 'src/models/network/INetwork';

const store = useNetStateStore();

const props = defineProps({
  // Allow specify EITHER network or networkId, but not both, and not neither
  network: { type: Object, required: false }, // Must be INetwork - in Vue 3.3 - https://github.com/vuejs/core/issues/4294
  networkId: { type: String, required: false },
  includeLinkToDetailsPage: { type: Boolean, required: false, default: false },
  showExtraDetails: { type: Boolean, required: false, default: false },
});

let polling: undefined | NodeJS.Timeout;

let allDevices: ComputedRef<IDevice[]> = computed(() => store.getDevices);

let currentNetwork = computed<INetwork | undefined>(
  // cast as INetwork no longer needed once we fix declation of props.network
  () => (props.network as INetwork) || store.getNetwork(props.networkId)
);

function doFetches() {
  if (props.network === undefined) {
    store.fetchNetworks([props.networkId]);
  }
  store.fetchActiveDevices();
  if (currentNetwork.value != null) {
    if (currentNetwork.value.aggregatesIrreversibly != null) {
      store.fetchNetworks(currentNetwork.value.aggregatesIrreversibly);
    }
    if (currentNetwork.value.aggregatesReversibly != null) {
      store.fetchNetworks(currentNetwork.value.aggregatesReversibly);
    }
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
  }, 15 * 1000);
});

onUnmounted(() => {
  clearInterval(polling);
});

function SortNetworkIDsByMostRecentFirst_(ids: Array<string>): Array<string> {
  let r: Array<string> = ids.filter((x) => true);
  r.sort((l, r) => {
    let lSeen = store.getNetwork(l)?.seen;
    let rSeen = store.getNetwork(r)?.seen;
    if (lSeen?.upperBound == null) {
      return 1;
    }
    if (rSeen?.upperBound == null) {
      return -1;
    }
    return moment(rSeen.upperBound).diff(lSeen.upperBound);
  });
  return r;
}

function GetSubNetworkDisplay_(id: string, summaryOnly: boolean): string {
  return FormatIDateTimeRange (store.getNetwork(id)?.seen, summaryOnly) ?? id;
}
</script>

<template>
  <div v-if="currentNetwork" class="q-pa-sm">
    <Link2DetailsPage :link="'/#/network/' + currentNetwork.id" v-if="props.includeLinkToDetailsPage"
      style="padding-top: 5px; float: right" />

    <div class="row">
      <div class="col-3">Name</div>
      <div class="col">
        {{
            currentNetwork.names.length > 0 ? currentNetwork.names[0].name : ''
        }}
      </div>
    </div>
    <div class="row" v-if="currentNetwork.names.filter((m) => m.priority > 1).length > 1">
      <div class="col-3">
        {{
            PluralizeNoun(
              'Alias',
              currentNetwork.names.filter((m) => m.priority > 1).length - 1
            )
        }}
      </div>
      <div class="col">
        {{
            currentNetwork.names
              .filter((m) => m.priority > 1)
              .map((m) => m.name)
              .slice(1)
              .join(', ')
        }}
      </div>
    </div>
    <div class="row">
      <div class="col-3">ID</div>
      <div class="col">
        {{ currentNetwork.id }}
        <span class="snapshot" v-if="currentNetwork.aggregatedBy">{snapshot}</span>
      </div>
    </div>
    <div class="row" v-if="currentNetwork.seen">
      <div class="col-3">Seen</div>
      <div class="col">
      {{FormatIDateTimeRange(currentNetwork.seen)}}
      </div>
    </div>
    <div class="row">
      <div class="col-3">
        {{ PluralizeNoun('CIDR', currentNetwork.networkAddresses.length) }}
      </div>
      <div class="col">{{ GetNetworkCIDRs(currentNetwork) }}</div>
    </div>
    <div class="row" v-if="currentNetwork.geographicLocation">
      <div class="col-3">Geographic Location</div>
      <div class="col">
        {{ FormatLocation(currentNetwork.geographicLocation) ?? '?' }}
      </div>
    </div>
    <div class="row" v-if="currentNetwork.internetServiceProvider">
      <div class="col-3">Internet Service Provider</div>
      <div class="col">{{ currentNetwork.internetServiceProvider.name }}</div>
    </div>
    <div class="row" v-if="currentNetwork.historicalSnapshot != true">
      <div class="col-3">
        {{
            PluralizeNoun(
              'Device',
              GetDeviceIDsInNetwork(currentNetwork, allDevices).length
            )
        }}
      </div>
      <div class="col">
        <a :href="GetDevicesForNetworkLink(currentNetwork.id)">{{
            GetDeviceIDsInNetwork(currentNetwork, allDevices).length
        }}</a>
      </div>
    </div>
    <div class="row" v-if="
      currentNetwork.externalAddresses &&
      currentNetwork.externalAddresses.length
    ">
      <div class="col-3">
        External IP
        {{ PluralizeNoun('Address', currentNetwork.externalAddresses.length) }}
      </div>
      <div class="col">{{ currentNetwork.externalAddresses.join(', ') }}</div>
    </div>
    <div class="row" v-if="
      (currentNetwork.gateways && currentNetwork.gateways.length) ||
      (currentNetwork.gatewayHardwareAddresses &&
        currentNetwork.gatewayHardwareAddresses.length)
    ">
      <div class="col-3">
        Gateway (IP/Hardware)
        {{
            PluralizeNoun(
              'Address',
              Math.max(
                currentNetwork.gateways?.length,
                currentNetwork.gatewayHardwareAddresses?.length
              )
            )
        }}
      </div>
      <div class="col">
        {{ currentNetwork.gateways?.join(', ') }} /
        {{ currentNetwork.gatewayHardwareAddresses?.join(', ') }}
      </div>
    </div>
    <div class="row" v-if="currentNetwork.DNSServers && currentNetwork.DNSServers.length">
      <div class="col-3">
        {{ PluralizeNoun('DNS Server', currentNetwork.DNSServers.length) }}
      </div>
      <div class="col">{{ currentNetwork.DNSServers.join(', ') }}</div>
    </div>
    <div class="row" v-if="
      currentNetwork.aggregatesReversibly &&
      currentNetwork.aggregatesReversibly.length
    ">
      <div class="col-3">Aggregates Reversibly</div>
      <div class="col">
        <div class="row wrap">
          <span v-for="aggregate in SortNetworkIDsByMostRecentFirst_(
            currentNetwork.aggregatesReversibly
          )" v-bind:key="aggregate" class="aggregatesItem">
            <ReadOnlyTextWithHover :message="GetSubNetworkDisplay_(aggregate, true)"
              :popup-title="GetSubNetworkDisplay_(aggregate, false)" :link="'/#/network/' + aggregate" />;&nbsp;
          </span>
        </div>
      </div>
    </div>
    <div class="row" v-if="
      currentNetwork.aggregatesIrreversibly &&
      currentNetwork.aggregatesIrreversibly.length
    ">
      <div class="col-3">Aggregates Irreversibly</div>
      <div class="col">
        <div class="row wrap">
          <span v-for="aggregate in SortNetworkIDsByMostRecentFirst_(
            currentNetwork.aggregatesIrreversibly
          )" v-bind:key="aggregate" class="aggregatesItem">
            <ReadOnlyTextWithHover :message="GetSubNetworkDisplay_(aggregate, true)"
              :popup-title="GetSubNetworkDisplay_(aggregate, false)" :link="'/#/network/' + aggregate" />;&nbsp;
          </span>
        </div>
      </div>
    </div>
    <div class="row" v-if="currentNetwork.attachedInterfaces && props.showExtraDetails">
      <div class="col-3">
        Attached Network
        {{
            PluralizeNoun('Interface', currentNetwork.attachedInterfaces.length)
        }}
      </div>
      <div class="col">
        <NetworkInterfacesDetails :network-interface-ids="currentNetwork.attachedInterfaces" />
      </div>
    </div>
    <div class="row" v-if="currentNetwork.debugProps && props.showExtraDetails">
      <div class="col-3">DEBUG INFO</div>
      <div class="col">
        <JsonViewer :value="currentNetwork.debugProps" :expand-depth="0" copyable sort class="debugInfoJSONViewers" />
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
