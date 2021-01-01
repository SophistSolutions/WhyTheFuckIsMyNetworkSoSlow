<template>
  <div>
    <table class="deviceDetailsTable" v-bind:key="device.id">
      <tr>
        <td class="labelColumn">Name (ID)</td>
        <td>
          <router-link v-bind:to="'/device/' + device.id">{{ device.name }}</router-link> ({{
            device.id
          }})
        </td>
      </tr>
      <tr v-if="device.type">
        <td class="labelColumn">Types</td>
        <td>{{ device.type.join(", ") }}</td>
      </tr>
      <tr v-if="device.icon">
        <td>Icon</td>
        <td><img :src="device.icon" /></td>
      </tr>
      <tr v-if="device.presentationURL">
        <td class="labelColumn">Presentation</td>
        <td>
          <a v-bind:href="device.presentationURL" target="_blank">{{ device.presentationURL }}</a>
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
          <ul>
            <li v-for="attachedNetID in Object.keys(device.attachedNetworks)">
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
                  <td>{{ device.attachedNetworks[attachedNetID].networkAddresses.join(", ") }}</td>
                </tr>
              </table>
            </li>
          </ul>
        </td>
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

  private formatNetworkAddresses_(attachedNetworks: {
    [key: string]: INetworkAttachmentInfo;
  }): string {
    let addresses: string[] = [];
    Object.entries(attachedNetworks).forEach((element) => {
      // console.log(element);
      element[1].networkAddresses.forEach((e: string) => addresses.push(e));
    });
    addresses = addresses.filter((value, index, self) => self.indexOf(value) === index);
    return addresses.join(", ");
  }
  private get deviceDetails(): object {
    const d = this.device;
    return {
      ...d,
      ...{
        localAddresses: this.formatNetworkAddresses_(d.attachedNetworks),
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
.deviceDetailsTable {
  table-layout: fixed;
}
.deviceDetailsTable td {
  padding-left: 5px;
  padding-right: 10px;
}
</style>
