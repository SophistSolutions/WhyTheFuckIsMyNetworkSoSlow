<script setup lang="ts">
import { defineProps, defineComponent, onMounted, onUnmounted, ref, computed } from 'vue';
import moment from 'moment';
import JsonViewer from 'vue-json-viewer';

import { IDevice, INetworkAttachmentInfo } from "../models/device/IDevice";
import { ComputeServiceTypeIconURL } from "../models/device/Utils";
import { GetNetworkLink, GetNetworkName, GetServices } from "../models/network/Utils";
import { rescanDevice } from "../proxy/API";


import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';
import Link2DetailsPage from '../components/Link2DetailsPage.vue';

import { useNetStateStore } from '../stores/Net-State-store'

const store = useNetStateStore()

const props = defineProps({
  deviceId: { type: String, required: true },
  includeLinkToDetailsPage: { type: Boolean, required: false, default: false },
})

defineComponent({
  components: {
    ReadOnlyTextWithHover,
    Link2DetailsPage,
    JsonViewer,
  },
});

let polling: undefined | NodeJS.Timeout;
var isRescanning: boolean = false;

function localNetworkAddresses(): string[] {
  const addresses: string[] = [];
  if (currentDevice.value) {
    Object.entries(currentDevice.value.attachedNetworks).forEach((element) => {
      element[1].localAddresses.forEach((e: string) => addresses.push(e));
    });
  }
  return addresses.filter((value, index, self) => self.indexOf(value) === index);
}

onMounted(() => {
  // first time check immediately, then more gradually for updates
  store.fetchDevice(props.deviceId);
  if (currentDevice.value) {
    store.fetchNetworks(Object.keys(currentDevice.value.attachedNetworks));
  } else {
    store.fetchAvailableNetworks();
  }
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    store.fetchDevice(props.deviceId);
    if (currentDevice.value) {
      store.fetchNetworks(Object.keys(currentDevice.value.attachedNetworks));
    }
  }, 15 * 1000);
})

onUnmounted(() => {
  clearInterval(polling);
})

async function rescanSelectedDevice(): Promise<void> {
  isRescanning = true;
  try {
    await rescanDevice(props.deviceId);
    store.fetchDevice(props.deviceId);
  } finally {
    isRescanning = false;
  }
}

const currentDevice = computed<IDevice | undefined>(
  () => store.getDevice(props.deviceId)
)


interface IExtendedDevice extends IDevice {
  localAddresses: string;
  attachedNetworks: any;
}

let currentDeviceDetails = computed<IExtendedDevice | undefined>(
  () => {
    if (currentDevice.value) {
      const attachedNetworkInfo = {} as { [key: string]: object };
      Object.keys(currentDevice.value.attachedNetworks).forEach((element: any) => {
        const thisNWI = currentDevice.value.attachedNetworks[element] as INetworkAttachmentInfo;
        let netName = "?";
        const thisNetObj = store.getNetwork(element);
        if (thisNetObj) {
          netName = GetNetworkName(thisNetObj);
        }
        attachedNetworkInfo[element] = { ...thisNWI, name: netName };
      });
      return {
        ...currentDevice.value,
        ...{
          localAddresses: localNetworkAddresses().join(", "),
          attachedNetworks: attachedNetworkInfo,
        },
      };
    }
    return undefined;
  }
)
</script>

<template>
  <div v-if="currentDevice" class="q-pa-sm">
    <Link2DetailsPage :link="'/#/device/' + currentDevice.id" v-if="props.includeLinkToDetailsPage" />

    <div class="row">
      <div class="col-3 labelColumn"> Name </div>
      <div class="col"> {{ currentDevice.name }} </div>
    </div>
    <div class="row">
      <div class="col-3 labelColumn"> ID </div>
      <div class="col"> {{ currentDevice.id }} <span class="snapshot"
          v-if="currentDevice.historicalSnapshot == true">{snapshot}</span></div>
    </div>
    <div class="row" v-if="currentDevice.type">
      <div class="col-3 labelColumn"> Types </div>
      <div class="col"> {{ currentDevice.type.join(", ") }}</div>
    </div>
    <div class="row" v-if="currentDevice.icon">
      <div class="col-3 labelColumn"> Icon </div>
      <div class="col"> <img :src="currentDevice.icon" width="24" height="24" /></div>
    </div>
    <div class="row" v-if="currentDevice.manufacturer">
      <div class="col-3 labelColumn"> Manufacturer </div>
      <div class="col">
        <span v-if="currentDevice.manufacturer.shortName || currentDevice.manufacturer.fullName">{{
            currentDevice.manufacturer.shortName || currentDevice.manufacturer.fullName
        }}</span>
        <span v-if="currentDevice.manufacturer.webSiteURL">
          <span v-if="currentDevice.manufacturer.shortName || currentDevice.manufacturer.fullName">; </span>
          Link:
          <a :href="currentDevice.manufacturer.webSiteURL" target="_blank">{{
              currentDevice.manufacturer.webSiteURL
          }}</a>
        </span>
      </div>
    </div>
    <div class="row" v-if="currentDevice.operatingSystem">
      <div class="col-3 labelColumn"> OS </div>
      <div class="col"> {{ currentDevice.operatingSystem.fullVersionedName }}</div>
    </div>
    <div class="row" v-if="currentDevice.lastSeenAt">
      <div class="col-3 labelColumn"> Last Seen </div>
      <div class="col"> {{ moment(currentDevice.lastSeenAt).fromNow() }}</div>
    </div>
    <div class="row">
      <div class="col-3 labelColumn"> Networks </div>
      <div class="col">
        <div class="row" v-for="attachedNetID in Object.keys(currentDevice.attachedNetworks)"
          v-bind:key="attachedNetID">
          <div class="col">
            <div class="row">
              <div class="col no-wrap">
                <ReadOnlyTextWithHover :message="
                  currentDeviceDetails.attachedNetworks[attachedNetID].name +
                  ' (' +
                  attachedNetID +
                  ')'
                " :link="GetNetworkLink(attachedNetID)" title="Network Name" />
              </div>
            </div>
            <div class="row" v-if="currentDevice.attachedNetworks[attachedNetID].hardwareAddresses">
              <div class="col-1" />
              <div class="col-4">Hardware Address(es)</div>
              <div class="col no-wrap"> {{ currentDevice.attachedNetworks[attachedNetID].hardwareAddresses.join(", ") }}
              </div>
            </div>
            <div class="row" v-if="currentDevice.attachedNetworks[attachedNetID].localAddresses">
              <div class="col-1" />
              <div class="col-4">Network Address Binding(s)</div>
              <div class="col no-wrap"> {{ currentDevice.attachedNetworks[attachedNetID].localAddresses.join(", ") }}
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
    <div class="row">
      <div class="col-3 labelColumn"> Services </div>
      <div class="col">
        <div class="row" v-for="svc in GetServices(currentDevice)" v-bind:key="svc.name">
          <div class="col-1">
            <img v-if="ComputeServiceTypeIconURL(svc.name).url" :src="ComputeServiceTypeIconURL(svc.name).url"
              height="20" width="20" />
          </div>
          <div class="col-1">{{ svc.name }}</div>
          <div class="col no-wrap"> <a v-for="l in svc.links" v-bind:href="l.href" v-bind:key="l.href"
              class="list-items" target="_blank">{{
                  l.href
              }}</a>
          </div>
        </div>
      </div>
    </div>
    <div class="row">
      <div class="col-3 labelColumn"> Open Ports </div>
      <div class="col">
        <q-btn class="smallBtnMargin" elevation="2" dense x-small @click="rescanSelectedDevice"
          :disabled="isRescanning"> Rescan </q-btn>
        <span v-if="currentDevice.openPorts">{{ currentDevice.openPorts.join(", ") }}</span>
      </div>
    </div>
    <div class="row" v-if="currentDevice.attachedNetworkInterfaces">
      <div class="col-3 labelColumn"> ATTACHED NETWORK INTERFACES </div>
      <div class="col">
        <json-viewer :value="currentDevice.attachedNetworkInterfaces" :expand-depth="0" copyable sort
          class="debugInfoJSONViewers" />
      </div>
    </div>
    <div class="row" v-if="currentDevice.aggregatesReversibly && currentDevice.aggregatesReversibly.length">
      <div class="col-3 labelColumn"> Aggregates Reversibly </div>
      <div class="col">
        <span v-for="aggregate in currentDevice.aggregatesReversibly" v-bind:key="aggregate">
          <ReadOnlyTextWithHover :message="aggregate" :link="'/#/device/' + aggregate" />;
        </span>
      </div>
    </div>
    <div class="row" v-if="currentDevice.aggregatesIrreversibly && currentDevice.aggregatesIrreversibly.length">
      <div class="col-3 labelColumn"> Aggregates Irreversibly </div>
      <div class="col">
        <span v-for="aggregate in currentDevice.aggregatesIrreversibly" v-bind:key="aggregate">
          <ReadOnlyTextWithHover :message="aggregate" />;
        </span>
      </div>
    </div>
    <div class="row" v-if="currentDevice.debugProps">
      <div class="col-3 labelColumn"> DEBUG INFO </div>
      <div class="col">
        <json-viewer :value="currentDevice.debugProps" :expand-depth="0" copyable sort class="debugInfoJSONViewers" />
      </div>
    </div>
  </div>
</template>

<style scoped lang="scss">
.list-items {
  padding-right: 1em;
}

.labelColumn {
  vertical-align: top;
}

.nowrap {
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.smallBtnMargin {
  margin-left: 1em;
  margin-right: 1em;
}

.snapshot {
  font-style: italic;
}

.debugInfoJSONViewers {
  margin-right: 2em;
}
</style>