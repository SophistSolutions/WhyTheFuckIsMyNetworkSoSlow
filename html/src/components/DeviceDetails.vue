<template>
  <div class="device">
    <table v-bind:key="device.id">
      <tr>
        <td>Name</td>
        <td>{{ device.name }}</td>
      </tr>
      <tr>
        <td>ID</td>
        <td>
          <router-link v-bind:to="'/device/' + device.id">{{ device.id }}</router-link>
        </td>
      </tr>
      <tr v-if="device.type">
        <td>Types</td>
        <td>{{ device.type.join(", ") }}</td>
      </tr>
      <tr v-if="device.icon">
        <td>Icon</td>
        <td><img :src="device.icon" /></td>
      </tr>
      <tr v-if="device.openPorts">
        <td>Open Ports</td>
        <td>{{ device.openPorts.join(", ") }}</td>
      </tr>
      <tr v-if="device.presentationURL">
        <td>Presentation</td>
        <td>
          <a v-bind:href="device.presentationURL" target="_blank">{{ device.presentationURL }}</a>
        </td>
      </tr>
      <tr v-if="device.operatingSystem">
        <td>OS</td>
        <td>
          {{ device.operatingSystem.fullVersionedName }}
        </td>
      </tr>
      <tr>
        <td>Networks</td>
        <td>
          {{ device.attachedNetworks }}
        </td>
      </tr>

      <tr>
        <td>Internet Addresses</td>
        <td>{{ getDeviceNetworkAddresses_(device) }}</td>
      </tr>
      <tr>
        <td>Hardware Addresses</td>
        <td>{{ getDeviceHardwareAddresses_(device) }}</td>
      </tr>
      <tr v-if="device.manufacturer">
        <td>
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
      <tr>
        <td>Last Seen</td>
        <td>{{ device.lastSeenAt | moment("from", "now") }}</td>
      </tr>
      <tr v-if="device.debugProps">
        <td>DEBUG INFO</td>
        <td>{{ device.debugProps }}</td>
      </tr>
    </table>
  </div>
</template>

<script lang="ts">
import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import { INetwork } from "@/models/network/INetwork";
import { OperatingSystem } from "@/models/OperatingSystem";
import { Component, Prop, Vue, Watch } from "vue-property-decorator";

@Component({
  name: "DeviceDetails",
})
export default class DeviceDetails2 extends Vue {
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

  private getDeviceHardwareAddresses_(d: IDevice) {
    let result = "";
    for (const value of Object.entries(d.attachedNetworks)) {
      if (result !== "") {
        result += ", ";
      }
      result += value[1].hardwareAddresses.join(", ");
    }
    return result;
  }
  private getDeviceNetworkAddresses_(d: IDevice) {
    let result = "";
    for (const value of Object.entries(d.attachedNetworks)) {
      if (result !== "") {
        result += ", ";
      }
      result += value[1].networkAddresses.join(", ");
    }
    return result;
  }
  private formatNetworks_(attachedNetworks: { [key: string]: INetworkAttachmentInfo }): string {
    let addresses: string[] = [];
    Object.entries(attachedNetworks).forEach((element) => {
      let netID = element[0];
      this.networks.forEach((network: INetwork) => {
        if (network.id === netID) {
          if (network.networkAddresses.length >= 1) {
            netID = network.networkAddresses[0];
          }
        }
      });
      addresses.push(netID);
    });
    addresses = addresses.filter((value, index, self) => self.indexOf(value) === index);
    return addresses.join(", ");
  }
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
        networksSummary: this.formatNetworks_(d.attachedNetworks),
        localAddresses: this.formatNetworkAddresses_(d.attachedNetworks),
        manufacturerSummary:
          d.manufacturer == null ? "" : d.manufacturer.fullName || d.manufacturer.shortName,
      },
    };
  }
}
</script>

<style scoped lang="scss"></style>
