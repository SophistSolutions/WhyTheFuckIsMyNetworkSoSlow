<script setup lang="ts">
import { defineProps, defineComponent, onMounted, onUnmounted, Ref, ref, computed } from 'vue';
import moment from 'moment';
import JsonViewer from 'vue-json-viewer';

import { IDevice, INetworkAttachmentInfo } from "../models/device/IDevice";
import { ComputeServiceTypeIconURL } from "../models/device/Utils";
import { GetNetworkLink, GetNetworkName, GetServices, SortNetworks } from "../models/network/Utils";
import { rescanDevice } from "../proxy/API";


import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';
import Link2DetailsPage from '../components/Link2DetailsPage.vue';

import { useNetStateStore } from '../stores/Net-State-store'
import { INetwork } from 'src/models/network/INetwork';

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
var isRescanning: Ref<boolean> = ref(false);

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
  isRescanning.value = true;
  try {
    await rescanDevice(props.deviceId);
    store.fetchDevice(props.deviceId);
  } finally {
    isRescanning.value = false;
  }
}

const currentDevice = computed<IDevice | undefined>(
  () => store.getDevice(props.deviceId)
)


interface IExtendedDevice extends IDevice {
  localAddresses: string;
  attachedNetworks: any;
  attachedFullNetworkObjects: INetwork[];
}

let currentDeviceDetails = computed<IExtendedDevice | undefined>(
  () => {
    if (currentDevice.value) {
      let attachedFullNetworkObjects = [] as INetwork[];
      let attachedNetworkInfo = {} as { [key: string]: object };
      Object.keys(currentDevice.value.attachedNetworks).forEach((element: any) => {
        const thisNWI = currentDevice.value.attachedNetworks[element] as INetworkAttachmentInfo;
        let netName = "?";
        const thisNetObj : INetwork = store.getNetwork(element);
        if (thisNetObj) {
          netName = GetNetworkName(thisNetObj);
          attachedFullNetworkObjects.push (thisNetObj);
        }
        attachedNetworkInfo[element] = { ...thisNWI, name: netName };
      });
      attachedFullNetworkObjects = SortNetworks(attachedFullNetworkObjects)
      return {
        ...currentDevice.value,
        ...{
          localAddresses: localNetworkAddresses().join(", "),
          attachedNetworks: attachedNetworkInfo,
          attachedFullNetworkObjects: attachedFullNetworkObjects
        },
      };
    }
    return undefined;
  }
)
</script>

<template>
  <div v-if="currentDevice" class="q-pa-sm">
    <Link2DetailsPage :link="'/#/device/' + currentDevice.id" v-if="props.includeLinkToDetailsPage" style="padding-top: 5px; float:right" />

    <div class="row">
      <div class="col-3">Name</div>
      <div class="col"> {{ currentDevice.name }} </div>
    </div>
    <div class="row">
      <div class="col-3">ID</div>
      <div class="col"> {{ currentDevice.id }} <span class="snapshot"
          v-if="currentDevice.historicalSnapshot == true">{snapshot}</span></div>
    </div>
    <div class="row" v-if="currentDevice.type">
      <div class="col-3">Types</div>
      <div class="col"> {{ currentDevice.type.join(", ") }}</div>
    </div>
    <div class="row" v-if="currentDevice.icon">
      <div class="col-3">Icon</div>
      <div class="col"> <img :src="currentDevice.icon" width="24" height="24" /></div>
    </div>
    <div class="row" v-if="currentDevice.manufacturer">
      <div class="col-3">Manufacturer</div>
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
      <div class="col-3">OS</div>
      <div class="col"> {{ currentDevice.operatingSystem.fullVersionedName }}</div>
    </div>
    <div class="row" v-if="currentDevice.lastSeenAt">
      <div class="col-3">Last Seen</div>
      <div class="col"> {{ moment(currentDevice.lastSeenAt).fromNow() }}</div>
    </div>
    <div class="row" v-if="currentDevice.attachedNetworks && currentDeviceDetails">
      <div class="col-3">Networks</div>
      <div class="col">
        <div class="row" v-for="attachedNet in currentDeviceDetails.attachedFullNetworkObjects"
          v-bind:key="attachedNet.id">
          <div class="col">
            <div class="row">
              <div class="col no-wrap truncateWithElipsis">
                <ReadOnlyTextWithHover :message="
                  currentDeviceDetails.attachedNetworks[attachedNet.id].name +
                  ' (' +
                  attachedNet.id +
                  ')'
                " :link="GetNetworkLink(attachedNet.id)" title="Network Name" />
              </div>
            </div>
            <div class="row" v-if="currentDevice.attachedNetworks[attachedNet.id].hardwareAddresses">
              <div class="col-1" />
              <div class="col-4">Hardware Address(es)</div>
              <div class="col no-wrap truncateWithElipsis"> {{ currentDevice.attachedNetworks[attachedNet.id].hardwareAddresses.join(", ") }}
              </div>
            </div>
            <div class="row" v-if="currentDevice.attachedNetworks[attachedNet.id].localAddresses">
              <div class="col-1" />
              <div class="col-4">Network Address Binding(s)</div>
              <div class="col no-wrap truncateWithElipsis"> {{ currentDevice.attachedNetworks[attachedNet.id].localAddresses.join(", ") }}
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
    <div class="row">
      <div class="col-3">Services</div>
      <div class="col">
        <div class="row" v-for="svc in GetServices(currentDevice)" v-bind:key="svc.name">
          <div class="col-1">
            <img v-if="ComputeServiceTypeIconURL(svc.name).url" :src="ComputeServiceTypeIconURL(svc.name).url"
              height="20" width="20" />
          </div>
          <div class="col-1">{{ svc.name }}</div>
          <div class="col"> <div class="row wrap"> <a v-for="l in svc.links" v-bind:href="l.href" v-bind:key="l.href"
              class="list-items" target="_blank">{{
                  l.href
              }}</a></div>
          </div>
        </div>
      </div>
    </div>
    <div class="row">
      <div class="col-3">Open Ports</div>
      <div class="col">
        <q-btn class="smallBtnMargin" elevation="2" dense size="sm" @click="rescanSelectedDevice"
          :disabled="isRescanning"> {{ isRescanning ? "**SCANNING**" : "Rescan" }} </q-btn>
        <span v-if="currentDevice.openPorts">{{ currentDevice.openPorts.join(", ") }}</span>
      </div>
    </div>
    <div class="row" v-if="currentDevice.attachedNetworkInterfaces">
      <div class="col-3">ATTACHED NETWORK INTERFACES</div>
      <div class="col">
        <json-viewer :value="currentDevice.attachedNetworkInterfaces" :expand-depth="0" copyable sort
          class="debugInfoJSONViewers" />
      </div>
    </div>
    <div class="row" v-if="currentDevice.aggregatesReversibly && currentDevice.aggregatesReversibly.length">
      <div class="col-3">Aggregates Reversibly</div>
      <div class="col">
        <div class="row wrap"><span v-for="aggregate in currentDevice.aggregatesReversibly" v-bind:key="aggregate">
            <ReadOnlyTextWithHover :message="aggregate" :link="'/#/device/' + aggregate" />;&nbsp;
          </span></div>
      </div>
    </div>
    <div class="row" v-if="currentDevice.aggregatesIrreversibly && currentDevice.aggregatesIrreversibly.length">
      <div class="col-3">Aggregates Irreversibly</div>
      <div class="col">
        <div class="row wrap"> <span v-for="aggregate in currentDevice.aggregatesIrreversibly" v-bind:key="aggregate">
            <ReadOnlyTextWithHover :message="aggregate" />;&nbsp;
          </span></div>
      </div>
    </div>
    <div class="row" v-if="currentDevice.debugProps">
      <div class="col-3">DEBUG INFO</div>
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

.smallBtnMargin {
  margin-left: 1em;
  margin-right: 1em;
}

.snapshot {
  font-style: italic;
}
</style>