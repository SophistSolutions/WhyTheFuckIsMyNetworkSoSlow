<script setup lang="ts">
import { defineProps, defineComponent, onMounted, onUnmounted, ref, computed } from 'vue';
import * as moment from 'moment';
import JsonViewer from 'vue-json-viewer';

import { IDevice, INetworkAttachmentInfo } from "../models/device/IDevice";
import { ComputeServiceTypeIconURL } from "../models/device/Utils";
import { GetNetworkLink, GetNetworkName, GetServices } from "../models/network/Utils";
import { rescanDevice } from "../proxy/API";


import ReadOnlyTextWithHover from '../components/ReadOnlyTextWithHover.vue';

import { useWTFStore } from '../stores/WTF-store'

const store = useWTFStore()

const props = defineProps({
  deviceId: { type: String, required: true },
})

defineComponent({
  components: {
    ReadOnlyTextWithHover,
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

let currentDevice = computed<IDevice | undefined>(
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
  <div v-if="currentDevice">
    <table class="detailsTable" v-bind:key="currentDevice.id">
      <tr>
        <td class="labelColumn">Name</td>
        <td>{{ currentDevice.name }}</td>
      </tr>
      <tr>
        <td class="labelColumn">ID</td>
        <td>
          {{ currentDevice.id }}
          <span class="snapshot" v-if="currentDevice.historicalSnapshot == true">{snapshot}</span>
        </td>
      </tr>
      <tr v-if="currentDevice.type">
        <td class="labelColumn">Types</td>
        <td>{{ currentDevice.type.join(", ") }}</td>
      </tr>
      <tr v-if="currentDevice.icon">
        <td>Icon</td>
        <td><img :src="currentDevice.icon" width="24" height="24" /></td>
      </tr>
      <tr v-if="currentDevice.manufacturer">
        <td class="labelColumn">Manufacturer</td>
        <td>
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
        </td>
      </tr>
      <tr v-if="currentDevice.operatingSystem">
        <td class="labelColumn">OS</td>
        <td>
          {{ currentDevice.operatingSystem.fullVersionedName }}
        </td>
      </tr>
      <tr v-if="currentDevice.lastSeenAt">
        <td class="labelColumn">Last Seen</td>
        <td>{{ moment(currentDevice.lastSeenAt).fromNow() }}</td>
      </tr>
      <tr>
        <td class="labelColumn">Networks</td>
        <td>
          <table>
            <tr v-for="attachedNetID in Object.keys(currentDevice.attachedNetworks)" v-bind:key="attachedNetID">
              <td valign="top">&#x25cf;</td>
              <td>
                <table>
                  <tr>
                    <td>Name (ID)</td>
                    <td class="nowrap">
                      <ReadOnlyTextWithHover :message="
                        currentDeviceDetails.attachedNetworks[attachedNetID].name +
                        ' (' +
                        attachedNetID +
                        ')'
                      " :link="GetNetworkLink(attachedNetID)" />
                    </td>
                  </tr>
                  <tr v-if="currentDevice.attachedNetworks[attachedNetID].hardwareAddresses">
                    <td>Hardware Address(es)</td>
                    <td class="nowrap">
                      {{ currentDevice.attachedNetworks[attachedNetID].hardwareAddresses.join(", ") }}
                    </td>
                  </tr>
                  <tr v-if="currentDevice.attachedNetworks[attachedNetID].localAddresses">
                    <td>Network Address Binding(s)</td>
                    <td class="nowrap">
                      {{ currentDevice.attachedNetworks[attachedNetID].localAddresses.join(", ") }}
                    </td>
                  </tr>
                </table>
              </td>
            </tr>
          </table>
        </td>
      </tr>
      <tr>
        <td class="labelColumn">Services</td>
        <td>
          <table>
            <tr v-for="svc in GetServices(currentDevice)" v-bind:key="svc.name">
              <td>
                <img v-if="ComputeServiceTypeIconURL(svc.name).url" :src="ComputeServiceTypeIconURL(svc.name).url"
                  height="20" width="20" />
              </td>
              <td class="labelColumn">{{ svc.name }}</td>
              <td>
                <a v-for="l in svc.links" v-bind:href="l.href" v-bind:key="l.href" class="list-items" target="_blank">{{
                    l.href
                }}</a>
              </td>
            </tr>
            <tr v-if="GetServices(currentDevice).length == 0">
              <td><em>none</em></td>
            </tr>
          </table>
        </td>
      </tr>
      <tr>
        <td class="labelColumn">Open Ports</td>
        <td>
          <q-btn class="smallBtnMargin" elevation="2" x-small @click="rescanSelectedDevice" :disabled="isRescanning">
            Rescan
          </q-btn>
          <span v-if="currentDevice.openPorts">{{ currentDevice.openPorts.join(", ") }}</span>
        </td>
      </tr>
      <tr v-if="currentDevice.attachedNetworkInterfaces">
        <td class="labelColumn">ATTACHED NETWORK INTERFACES</td>
        <td>
          <json-viewer :value="currentDevice.attachedNetworkInterfaces" :expand-depth="0" copyable sort />
        </td>
      </tr>
      <tr v-if="currentDevice.aggregatesReversibly && currentDevice.aggregatesReversibly.length">
        <td>Aggregates Reversibly</td>
        <td>
          <span v-for="aggregate in currentDevice.aggregatesReversibly" v-bind:key="aggregate">
            <ReadOnlyTextWithHover :message="aggregate" :link="'/#/device/' + aggregate" />;
          </span>
        </td>
      </tr>
      <tr v-if="currentDevice.aggregatesIrreversibly && currentDevice.aggregatesIrreversibly.length">
        <td>Aggregates Irreversibly</td>
        <td>
          <span v-for="aggregate in currentDevice.aggregatesIrreversibly" v-bind:key="aggregate">
            <ReadOnlyTextWithHover :message="aggregate" />;
          </span>
        </td>
      </tr>
      <tr v-if="currentDevice.debugProps">
        <td class="labelColumn">DEBUG INFO</td>
        <td>
          <json-viewer :value="currentDevice.debugProps" :expand-depth="1" copyable sort></json-viewer>
        </td>
      </tr>
    </table>
  </div>
</template>

<style scoped lang="scss">
.list-items {
  padding-right: 1em;
}

td.labelColumn {
  vertical-align: top;
}

.detailsTable {
  table-layout: fixed;
}

.detailsTable td {
  padding-left: 5px;
  padding-right: 10px;
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
</style>