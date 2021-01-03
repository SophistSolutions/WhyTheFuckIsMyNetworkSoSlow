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
        <td><img :src="device.icon" /></td>
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
                    <td class=".flex-nowrap">
                      <a :href="GetNetworkLink(attachedNetID)">
                        {{ GetNetworkName(GetNetworkByID(attachedNetID, networks)) }}</a
                      >
                      ({{ attachedNetID }})
                    </td>
                  </tr>
                  <tr v-if="device.attachedNetworks[attachedNetID].hardwareAddresses">
                    <td>Hardware Address(es)</td>
                    <td class=".flex-nowrap">
                      {{ device.attachedNetworks[attachedNetID].hardwareAddresses.join(", ") }}
                    </td>
                  </tr>
                  <tr v-if="device.attachedNetworks[attachedNetID].networkAddresses">
                    <td>Network Address Binding(s)</td>
                    <td>
                      {{ device.attachedNetworks[attachedNetID].networkAddresses.join(", ") }}
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
            <tr v-if="device.presentationURL">
              <td>&#x25cf;</td>
              <td class="labelColumn">Presentation</td>
              <td>
                <a v-bind:href="device.presentationURL" target="_blank">{{
                  device.presentationURL
                }}</a>
              </td>
            </tr>
            <!-- see https://www.w3.org/wiki/UriSchemes -->
            <tr
              v-if="
                device.openPorts &&
                  device.openPorts.includes('tcp:3389') &&
                  localNetworkAddresses.length > 0
              "
            >
              <td>&#x25cf;</td>
              <td>rdp</td>
              <td>
                <a v-for="la in localNetworkAddresses" :href="'rdp://' + la">{{ "rdp://" + la }}</a>
              </td>
            </tr>
            <tr
              v-if="
                device.openPorts &&
                  device.openPorts.includes('tcp:22') &&
                  localNetworkAddresses.length > 0
              "
            >
              <td>&#x25cf;</td>
              <td>ssh</td>
              <td>
                <a v-for="la in localNetworkAddresses" :href="'telnet:@' + la">{{
                  "telnet:@" + la
                }}</a>
              </td>
            </tr>
            <tr
              v-if="
                device.openPorts &&
                  device.openPorts.includes('tcp:139') &&
                  localNetworkAddresses.length > 0
              "
            >
              <td>&#x25cf;</td>
              <td>smb</td>
              <td>
                <a v-for="la in localNetworkAddresses" :href="'smb://' + la">{{ "smb://" + la }}</a>
              </td>
            </tr>
            <tr
              v-if="
                device.openPorts &&
                  device.openPorts.includes('tcp:80') &&
                  localNetworkAddresses.length > 0
              "
            >
              <td>&#x25cf;</td>
              <td>web</td>
              <td>
                <a v-for="la in localNetworkAddresses" :href="'http://@' + la">{{
                  "http://@" + la
                }}</a>
              </td>
            </tr>
            <tr
              v-if="
                device.openPorts &&
                  device.openPorts.includes('tcp:443') &&
                  localNetworkAddresses.length > 0
              "
            >
              <td>&#x25cf;</td>
              <td>web</td>
              <td>
                <a v-for="la in localNetworkAddresses" :href="'https://@' + la">{{
                  "https://@" + la
                }}</a>
              </td>
            </tr>
          </table>
        </td>
      </tr>
      <tr v-if="device.openPorts">
        <td class="labelColumn">Open Ports</td>
        <td>{{ device.openPorts.join(", ") }}</td>
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
import { INetwork } from "@/models/network/INetwork";
import { GetNetworkByID, GetNetworkLink, GetNetworkName } from "@/models/network/Utils";
import { OperatingSystem } from "@/models/OperatingSystem";
import { Component, Prop, Vue, Watch } from "vue-property-decorator";

@Component({
  name: "DeviceDetails",
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

  private GetNetworkName = GetNetworkName;
  private GetNetworkLink = GetNetworkLink;
  private GetNetworkByID = GetNetworkByID;

  private get localNetworkAddresses(): string[] {
    const addresses: string[] = [];
    Object.entries(this.device.attachedNetworks).forEach((element) => {
      element[1].networkAddresses.forEach((e: string) => addresses.push(e));
    });
    return addresses.filter((value, index, self) => self.indexOf(value) === index);
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
</style>
