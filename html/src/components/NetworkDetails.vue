<template>
  <div>
    <table class="detailsTable" v-bind:key="network.id">
      <tr>
        <td>Name (ID)</td>
        <td>{{ GetNetworkName(network) }} ({{ network.id }})</td>
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
          <json-viewer :value="network.attachedInterfaces" :expand-depth="0" copyable sort />
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
import {
  FormatLocation,
  GetDeviceIDsInNetwork,
  GetDevicesForNetworkLink,
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
  public devices!: IDevice[];

  private GetNetworkName = GetNetworkName;
  private FormatLocation = FormatLocation;
  private GetDeviceIDsInNetwork = GetDeviceIDsInNetwork;
  private GetDevicesForNetworkLink = GetDevicesForNetworkLink;
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
