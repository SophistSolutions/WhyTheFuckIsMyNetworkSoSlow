<template>
  <div>
    <table class="detailsTable" v-bind:key="network.id">
      <tr>
        <td>Name</td>
        <td>{{ GetNetworkName(network) }}</td>
      </tr>
      <tr>
        <td>ID</td>
        <td>{{ network.id }}</td>
      </tr>
      <tr>
        <td>Friendly Name</td>
        <td>{{ network.friendlyName }}</td>
      </tr>
      <tr v-if="network.lastSeenAt">
        <td>Last Seen</td>
        <td>{{ network.lastSeenAt | moment("from", "now") }}</td>
      </tr>
      <tr>
        <td>Networks</td>
        <td>{{ GetNetworkCIDRs(network) }}</td>
      </tr>
      <tr v-if="network.DNSServers && network.DNSServers.length">
        <td>DNS Servers</td>
        <td>{{ network.DNSServers.join(", ") }}</td>
      </tr>
      <tr v-if="network.gateways && network.gateways.length">
        <td>Gateways</td>
        <td>{{ network.gateways.join(", ") }}</td>
      </tr>
      <tr v-if="network.geographicLocation">
        <td>Geographic Location</td>
        <td>
          {{ FormatLocation(network.geographicLocation) }}
        </td>
      </tr>
      <tr v-if="network.internetServiceProvider">
        <td>Internet Service Provider</td>
        <td>{{ network.internetServiceProvider.name }}</td>
      </tr>
      <tr>
        <td>Devices</td>
        <td>
          <a :href="GetDevicesForNetworkLink(network.id)">{{
            GetDeviceIDsInNetwork(network, devices).length
          }}</a>
        </td>
      </tr>
      <tr v-if="network.attachedInterfaces">
        <td>ATTACHED INTERFACES</td>
        <td>
          <json-viewer :value="thisNetworksInterfaces" :expand-depth="0" copyable sort />
        </td>
      </tr>
      <tr v-if="network.debugProps">
        <td>DEBUG INFO</td>
        <td>
          <json-viewer :value="network.debugProps" :expand-depth="1" copyable sort />
        </td>
      </tr>
    </table>
  </div>
</template>

<script lang="ts">
import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import { IGeographicLocation } from "@/models/network/IGeographicLocation";
import { INetwork } from "@/models/network/INetwork";
import { INetworkInterface } from "@/models/network/INetworkInterface";
import {
  FormatLocation,
  GetDeviceIDsInNetwork,
  GetDevicesForNetworkLink,
  GetNetworkCIDRs,
  GetNetworkName,
} from "@/models/network/Utils";
import { OperatingSystem } from "@/models/OperatingSystem";
import { Component, Prop, Vue, Watch } from "vue-property-decorator";

@Component({
  name: "NetworkDetails",
})
export default class NetworkDetails extends Vue {
  @Prop({
    required: true,
    default: null,
  })
  public network!: INetwork;

  @Prop({
    required: true,
    default: null,
  })
  public networkInterfaces!: INetworkInterface[];

  @Prop({
    required: true,
    default: null,
  })
  public devices!: IDevice[];

  private GetNetworkName = GetNetworkName;
  private GetNetworkCIDRs = GetNetworkCIDRs;
  private FormatLocation = FormatLocation;
  private GetDeviceIDsInNetwork = GetDeviceIDsInNetwork;
  private GetDevicesForNetworkLink = GetDevicesForNetworkLink;

  private get thisNetworksInterfaces(): INetworkInterface[] {
    const result: INetworkInterface[] = [];
    this.network.attachedInterfaces.forEach((e) => {
      let answer: INetworkInterface = { id: e };
      this.networkInterfaces.forEach((ni) => {
        if (e === ni.id) {
          answer = ni;
        }
      });
      result.push(answer);
    });
    return result;
  }
}
</script>

<style scoped lang="scss">
.detailsTable {
  table-layout: fixed;
}
.detailsTable td {
  padding-left: 5px;
  padding-right: 10px;
}
</style>
