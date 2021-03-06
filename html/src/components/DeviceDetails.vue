<template>
  <div>
    <table class="detailsTable" v-bind:key="device.id">
      <tr>
        <td class="labelColumn">Name (ID)</td>
        <td>{{ device.name }} ({{ device.id }})</td>
      </tr>
      <tr v-if="device.type">
        <td class="labelColumn">Types</td>
        <td>{{ device.type.join(", ") }}</td>
      </tr>
      <tr v-if="device.icon">
        <td>Icon</td>
        <td><img :src="device.icon" width="24" height="24" /></td>
      </tr>
      <tr v-if="device.manufacturer">
        <td class="labelColumn">
          Manufacturer
        </td>
        <td>
          <span v-if="device.manufacturer.shortName || device.manufacturer.fullName">{{
            device.manufacturer.shortName || device.manufacturer.fullName
          }}</span>
          <span v-if="device.manufacturer.webSiteURL">
            <span v-if="device.manufacturer.shortName || device.manufacturer.fullName">; </span>
            Link:
            <a :href="device.manufacturer.webSiteURL" target="_blank">{{
              device.manufacturer.webSiteURL
            }}</a>
          </span>
        </td>
      </tr>
      <tr v-if="device.operatingSystem">
        <td class="labelColumn">OS</td>
        <td>
          {{ device.operatingSystem.fullVersionedName }}
        </td>
      </tr>
      <tr v-if="device.lastSeenAt">
        <td class="labelColumn">Last Seen</td>
        <td>{{ device.lastSeenAt | moment("from", "now") }}</td>
      </tr>
      <tr>
        <td class="labelColumn">Networks</td>
        <td>
          <table>
            <tr v-for="attachedNetID in Object.keys(device.attachedNetworks)">
              <td valign="top">&#x25cf;</td>
              <td>
                <table>
                  <tr>
                    <td>Name (ID)</td>
                    <td class="nowrap">
                      <ReadOnlyTextWithHover
                        :message="
                          GetNetworkName(GetNetworkByID(attachedNetID, networks)) +
                            ' (' +
                            attachedNetID +
                            ')'
                        "
                        :link="GetNetworkLink(attachedNetID)"
                      />
                    </td>
                  </tr>
                  <tr v-if="device.attachedNetworks[attachedNetID].hardwareAddresses">
                    <td>Hardware Address(es)</td>
                    <td class="nowrap">
                      {{ device.attachedNetworks[attachedNetID].hardwareAddresses.join(", ") }}
                    </td>
                  </tr>
                  <tr v-if="device.attachedNetworks[attachedNetID].localAddresses">
                    <td>Network Address Binding(s)</td>
                    <td class="nowrap">
                      {{ device.attachedNetworks[attachedNetID].localAddresses.join(", ") }}
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
            <tr v-for="svc in GetServices(device)">
              <td>
                <img
                  v-if="ComputeServiceTypeIconURL(svc.name).url"
                  :src="ComputeServiceTypeIconURL(svc.name).url"
                  height="20"
                  width="20"
                />
              </td>
              <td class="labelColumn">{{ svc.name }}</td>
              <td>
                <a v-for="l in svc.links" v-bind:href="l.href" class="list-items" target="_blank">{{
                  l.href
                }}</a>
              </td>
            </tr>
            <tr v-if="GetServices(device).length == 0">
              <td><em>none</em></td>
            </tr>
          </table>
        </td>
      </tr>
      <tr>
        <td class="labelColumn">Open Ports</td>
        <td>
          <v-btn
            class="smallBtnMargin"
            elevation="2"
            x-small
            @click="rescanDevice"
            :disabled="isRescanning"
            >Rescan</v-btn
          >
          <span v-if="device.openPorts">{{ device.openPorts.join(", ") }}</span>
        </td>
      </tr>
      <tr v-if="device.attachedNetworkInterfaces">
        <td class="labelColumn">ATTACHED NETWORK INTERFACES</td>
        <td>
          <json-viewer :value="device.attachedNetworkInterfaces" :expand-depth="0" copyable sort />
        </td>
      </tr>
      <tr v-if="device.debugProps">
        <td class="labelColumn">DEBUG INFO</td>
        <td>
          <json-viewer :value="device.debugProps" :expand-depth="1" copyable sort></json-viewer>
        </td>
      </tr>
    </table>
  </div>
</template>

<script lang="ts">
import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import {
  ComputeDeviceTypeIconURLs,
  ComputeOSIconURLList,
  ComputeServiceTypeIconURL,
} from "@/models/device/Utils";
import { INetwork } from "@/models/network/INetwork";
import {
  GetNetworkByID,
  GetNetworkLink,
  GetNetworkName,
  GetServices,
} from "@/models/network/Utils";
import { OperatingSystem } from "@/models/OperatingSystem";
import { Component, Prop, Vue, Watch } from "vue-property-decorator";
import { rescanDevice } from "@/proxy/API";

@Component({
  name: "DeviceDetails",
  components: {
    ReadOnlyTextWithHover: () => import("@/components/ReadOnlyTextWithHover.vue"),
  },
})
export default class DeviceDetails extends Vue {
  @Prop({
    required: true,
    default: null,
  })
  public device!: IDevice;

  @Prop({
    required: true,
    default: null,
  })
  public networks!: INetwork[];

  private isRescanning: boolean = false;

  private GetNetworkName = GetNetworkName;
  private GetNetworkLink = GetNetworkLink;
  private GetNetworkByID = GetNetworkByID;
  private GetServices = GetServices;
  private ComputeServiceTypeIconURL = ComputeServiceTypeIconURL;

  private get localNetworkAddresses(): string[] {
    const addresses: string[] = [];
    Object.entries(this.device.attachedNetworks).forEach((element) => {
      element[1].localAddresses.forEach((e: string) => addresses.push(e));
    });
    return addresses.filter((value, index, self) => self.indexOf(value) === index);
  }

  private async rescanDevice() {
    this.isRescanning = true;
    try {
      this.$store.dispatch("fetchDevices", null);
      await rescanDevice(this.device.id);
      this.$store.dispatch("fetchDevices", null);
    } finally {
      this.isRescanning = false;
    }
  }

  private get deviceDetails(): object {
    const d = this.device;
    return {
      ...d,
      ...{
        localAddresses: this.localNetworkAddresses.join(", "),
        manufacturerSummary:
          d.manufacturer == null ? "" : d.manufacturer.fullName || d.manufacturer.shortName,
      },
    };
  }
}
</script>

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
</style>
