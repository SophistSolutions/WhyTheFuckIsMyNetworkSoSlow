<template>
  <div class="network">
    <table v-bind:key="network.id">
      <tr>
        <td>ID</td>
        <td>
          <router-link v-bind:to="'/network/' + network.id">{{ network.id }}</router-link>
        </td>
      </tr>
      <tr v-if="network.networkAddresses && network.networkAddresses.length">
        <td>Name</td>
        <td>{{ network.networkAddresses.join(", ") }}</td>
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
          {{ formatLocation_(network.geographicLocation) }}
        </td>
      </tr>
      <tr v-if="network.internetServiceProvider">
        <td>Internet Service Provider</td>
        <td>{{ network.internetServiceProvider.name }}</td>
      </tr>
      <tr v-if="network.debugProps">
        <td>DEBUG INFO</td>
        <td>
          <json-viewer :value="network.debugProps" :expand-depth="1" copyable sort></json-viewer>
        </td>
      </tr>
    </table>
  </div>
</template>

<script lang="ts">
import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import { IGeographicLocation } from "@/models/network/IGeographicLocation";
import { INetwork } from "@/models/network/INetwork";
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

  private formatLocation_(l: IGeographicLocation): string {
    let result: string = "";
    if (l.city != null) {
      result += l.city;
    }
    if (l.regionCode != null) {
      if (result !== "") {
        result += " ";
      }
      result += l.regionCode;
    }
    if (l.countryCode != null) {
      if (result !== "") {
        result += ", ";
      }
      result += l.countryCode;
    }
    if (l.postalCode != null) {
      if (result !== "") {
        result += " ";
      }
      result += l.postalCode;
    }
    return result;
  }
}
</script>

<style scoped lang="scss"></style>