<script setup lang="ts">
import { defineProps, defineComponent, onMounted, onUnmounted, ref, computed, ComputedRef } from 'vue';

import JsonViewer from 'vue-json-viewer';
import moment from 'moment';

import { IDevice, INetworkAttachmentInfo } from "../models/device/IDevice";
import { INetworkInterface } from "../models/network/INetworkInterface";

import {
  FormatLocation,
  GetDeviceIDsInNetwork,
  GetDevicesForNetworkLink,
  GetNetworkCIDRs,
  GetNetworkName,
} from "../models/network/Utils";


// Components
import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';
import Link2DetailsPage from '../components/Link2DetailsPage.vue';

import { useNetStateStore } from '../stores/Net-State-store'
import { INetwork } from 'src/models/network/INetwork';

const store = useNetStateStore()

const props = defineProps({
  networkId: { type: String, required: true },
  includeLinkToDetailsPage: { type: Boolean, required: false, default: false },
})

defineComponent({
  components: {
    ReadOnlyTextWithHover,
    JsonViewer,
    Link2DetailsPage,
  },
});

let polling: undefined | NodeJS.Timeout;

let allDevices: ComputedRef<IDevice[]> = computed(() => store.getDevices);

let currentNetwork = computed<INetwork | undefined>(
  () => store.getNetwork(props.networkId)
)

let networkInterfaces = computed<INetworkInterface[]>(
  () => store.getNetworkInterfaces
)

let thisNetworksInterfaces = computed<INetworkInterface[]>(
  () => {
    const result: INetworkInterface[] = [];
    if (currentNetwork.value) {
      currentNetwork.value.attachedInterfaces.forEach((e) => {
        let answer: INetworkInterface | undefined;
        networkInterfaces.value.forEach((ni) => {
          if (e === ni.id) {
            answer = ni;
          }
        });
        if (answer === undefined) {
          answer = { id: e };
        }
        result.push(answer);
      });
    }
    return result;
  }
)

function doFetches() {
  store.fetchNetworks([props.networkId]);
  store.fetchActiveDevices();
  if (currentNetwork.value != null) {
    if (currentNetwork.value.aggregatesIrreversibly != null) {
      store.fetchNetworks(currentNetwork.value.aggregatesIrreversibly);
    }
    if (currentNetwork.value.aggregatesReversibly != null) {
      store.fetchNetworks(currentNetwork.value.aggregatesReversibly);
    }
  }
};

onMounted(() => {
  // first time check immediately, then more gradually for updates
  doFetches();
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    doFetches();
  }, 15 * 1000);
})

onUnmounted(() => {
  clearInterval(polling);
})

function SortNetworkIDsByMostRecentFirst_(ids: Array<string>): Array<string> {
  let r: Array<string> = ids.filter(x => true);
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
  let r = store.getNetwork(id);
  let shortEverText: string | null = null;
  let longEverText: string | null = null;
  if (r != null) {
    if (r && r.seen && r.seen) {
      const seenRange = r.seen
      if (seenRange.upperBound) {
        shortEverText = moment(seenRange.upperBound).fromNow();
      }
      longEverText = moment(seenRange.lowerBound).fromNow() + ' up until ' + (shortEverText ?? "?");
    }
  }
  if (summaryOnly && shortEverText != null) {
    return shortEverText;
  }
  if (longEverText == null) {
    return id;
  }
  return longEverText + "; ID: " + id;
}

</script>

<template>
  <div v-if="currentNetwork" class="q-pa-sm">
    <Link2DetailsPage :link="'/#/network/' + currentNetwork.id" v-if="props.includeLinkToDetailsPage"
      style="padding-top: 5px; float:right" />

    <div class="row">
      <div class="col-3">Name</div>
      <div class="col"> {{ GetNetworkName(currentNetwork) }} </div>
    </div>
    <div class="row" v-if="currentNetwork.names.filter(m => m.priority > 1).length > 1">
      <div class="col-3">Aliases</div>
      <div class="col"> {{ currentNetwork.names.filter(m => m.priority > 1).map (m => m.name).slice(1).join (", ") }}
      </div>
    </div>
    <div class="row">
      <div class="col-3">ID</div>
      <div class="col"> {{ currentNetwork.id }} <span class="snapshot"
          v-if="currentNetwork.historicalSnapshot == true">{snapshot}</span></div>
    </div>
    <div class="row" v-if="currentNetwork.seen">
      <div class="col-3">Seen</div>
      <div class="col"> {{ moment(currentNetwork.seen.lowerBound).fromNow() }} up until {{
          moment(currentNetwork.seen.upperBound).fromNow()
      }} </div>
    </div>
    <div class="row">
      <div class="col-3">CIDRs</div>
      <div class="col"> {{ GetNetworkCIDRs(currentNetwork) }} </div>
    </div>
    <div class="row" v-if="currentNetwork.DNSServers && currentNetwork.DNSServers.length">
      <div class="col-3">DNS Server{{currentNetwork.DNSServers.length>=2?"s":""}}</div>
      <div class="col"> {{ currentNetwork.DNSServers.join(", ") }} </div>
    </div>
    <div class="row" v-if="(currentNetwork.gateways && currentNetwork.gateways.length) || (currentNetwork.gatewayHardwareAddresses && currentNetwork.gatewayHardwareAddresses.length)">
      <div class="col-3">Gateway{{((currentNetwork.gateways?.length>=2)||(currentNetwork.gatewayHardwareAddresses?.length>=2))?"s":""}} (IP / Hardware)</div>
      <div class="col"> {{ currentNetwork.gateways?.join(", ") }} / {{ currentNetwork.gatewayHardwareAddresses?.join(", ") }} </div>
    </div>
    <div class="row" v-if="currentNetwork.externalAddresses && currentNetwork.externalAddresses.length">
      <div class="col-3">External IP Address{{currentNetwork.externalAddresses.length>=2?"es":""}}</div>
      <div class="col"> {{ currentNetwork.externalAddresses.join(", ") }} </div>
    </div>
    <div class="row" v-if="currentNetwork.geographicLocation">
      <div class="col-3">Geographic Location</div>
      <div class="col"> {{ FormatLocation(currentNetwork.geographicLocation) ?? "?" }} </div>
    </div>
    <div class="row" v-if="currentNetwork.internetServiceProvider">
      <div class="col-3">Internet Service Provider</div>
      <div class="col"> {{ currentNetwork.internetServiceProvider.name }} </div>
    </div>
    <div class="row" v-if="currentNetwork.aggregatesReversibly && currentNetwork.aggregatesReversibly.length">
      <div class="col-3">Aggregates Reversibly</div>
      <div class="col">
        <div class="row wrap"><span
            v-for="aggregate in SortNetworkIDsByMostRecentFirst_(currentNetwork.aggregatesReversibly)"
            v-bind:key="aggregate" class="aggregatesItem">
            <ReadOnlyTextWithHover :message="GetSubNetworkDisplay_(aggregate, true)"
              :popup-title="GetSubNetworkDisplay_(aggregate, false)" :link="'/#/network/' + aggregate" />;&nbsp;
          </span></div>
      </div>
    </div>
    <div class="row" v-if="currentNetwork.aggregatesIrreversibly && currentNetwork.aggregatesIrreversibly.length">
      <div class="col-3">Aggregates Irreversibly</div>
      <div class="col">
        <div class="row wrap"><span
            v-for="aggregate in SortNetworkIDsByMostRecentFirst_(currentNetwork.aggregatesIrreversibly)"
            v-bind:key="aggregate" class="aggregatesItem">
            <ReadOnlyTextWithHover :message="GetSubNetworkDisplay_(aggregate, true)"
              :popup-title="GetSubNetworkDisplay_(aggregate, false)" :link="'/#/network/' + aggregate" />;&nbsp;
          </span> </div>
      </div>
    </div>
    <div class="row" v-if="currentNetwork.historicalSnapshot != true">
      <div class="col-3">Devices</div>
      <div class="col"> <a :href="GetDevicesForNetworkLink(currentNetwork.id)">{{
          GetDeviceIDsInNetwork(currentNetwork, allDevices).length
      }}</a> </div>
    </div>
    <div class="row" v-if="currentNetwork.attachedInterfaces">
      <div class="col-3">ATTACHED INTERFACES</div>
      <div class="col">
        <json-viewer :value="thisNetworksInterfaces" :expand-depth="0" copyable sort class="debugInfoJSONViewers" />
      </div>
    </div>
    <div class="row" v-if="currentNetwork.debugProps">
      <div class="col-3">DEBUG INFO</div>
      <div class="col">
        <json-viewer :value="currentNetwork.debugProps" :expand-depth="0" copyable sort class="debugInfoJSONViewers" />
      </div>
    </div>
  </div>
</template>

<style scoped lang="scss">
.snapshot {
  font-style: italic;
}

.aggregatesItem {
  min-width: 10em
}
</style>