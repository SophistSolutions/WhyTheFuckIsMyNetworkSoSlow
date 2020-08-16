<template>
  <v-container class="devices">
    <v-card>
      <v-card-title>
        Devices
        <v-spacer></v-spacer>
      </v-card-title>
      <v-data-table
        dense
        v-model="selectedRows"
        :headers="headers"
        :items="devicesAsDisplayed"
        :single-select="true"
        :search="search"
        :sort-by="[]"
        :sort-desc="[false, true]"
        multi-sort
        item-key="id"
        class="elevation-1"
      >
        <template v-slot:item="{ item }">
          <tr
            :class="selectedRows.indexOf(item.id) > -1 ? 'yellow' : ''"
            @click="rowClicked($event, item)"
          >
            <td>{{ item.name }}</td>
            <td>{{ item.type }}</td>
            <td>{{ item.manufacturer }}</td>
            <td>{{ item.os }}</td>
            <td>{{ item.networks }}</td>
            <td>{{ item.localAddrresses }}</td>
          </tr>
        </template>
      </v-data-table>
    </v-card>
  </v-container>
</template>

<script lang="ts">
import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import { Component, Vue, Watch } from "vue-property-decorator";

import { compareValues } from "@/CustomSort";
import { fetchNetworks } from "@/proxy/API";

@Component({
  name: "Devices",
  components: {
    Device: () => import("@/components/Device-DEPRECATED.vue"),
  },
})
export default class Devices extends Vue {
  private polling: undefined | number = undefined;

  private selectedRows: string[] = [];

  private rowClicked(e: any, row: IDevice) {
    this.toggleSelection(row.id);
    // @todo fix proper handling of shift key !e.shiftKey && !
    if (!e.ctrlKey) {
      // single select unless shift key
      this.selectedRows = this.selectedRows.filter((selectedKeyID) => selectedKeyID === row.id);
    }
    // console.log(row);
  }
  private toggleSelection(keyID: string) {
    if (this.selectedRows.includes(keyID)) {
      this.selectedRows = this.selectedRows.filter((selectedKeyID) => selectedKeyID !== keyID);
    } else {
      this.selectedRows.push(keyID);
    }
  }

  private formatNetworks_(attachedNetworks: { [key: string]: INetworkAttachmentInfo }): string {
    const addresses: string[] = [];
    Object.entries(attachedNetworks).forEach((element) => {
      // console.log(element);
      addresses.push(element[0]);
    });
    return addresses.join(", ");
  }

  private formatNetworkAddresses_(attachedNetworks: {
    [key: string]: INetworkAttachmentInfo;
  }): string {
    const addresses: string[] = [];
    Object.entries(attachedNetworks).forEach((element) => {
      // console.log(element);
      element[1].networkAddresses.forEach((e: string) => addresses.push(e));
    });
    return addresses.join(", ");
  }

  private get search(): string {
    return this.$store.getters.getSearchString;
  }
  private get headers(): object[] {
    return [
      {
        text: "Name",
        align: "start",
        value: "name",
      },
      {
        text: "Type",
        value: "type",
      },
      {
        text: "Manufacturer",
        value: "manufacturer",
      },
      {
        text: "OS",
        value: "operatingSystem",
      },
      {
        text: "Network",
        value: "networks",
      },
      {
        text: "Local Address",
        value: "localAddrresses",
      },
    ];
  }

  private created() {
    this.fetchDevices();
    this.pollData();
  }

  private beforeDestroy() {
    clearInterval(this.polling);
  }

  private fetchDevices() {
    this.$store.dispatch("fetchDevices", null);
  }

  private pollData() {
    this.polling = setInterval(() => {
      this.fetchDevices();
    }, 10 * 1000);
  }

  private get devices(): IDevice[] {
    return this.$store.getters.getDevices;
  }

  @Watch("devices()")
  private get devicesAsDisplayed(): object[] {
    const result: object[] = [];
    this.devices.forEach((i) => {
      result.push({
        id: i.id,
        name: i.name,
        type: i.type == null ? null : i.type.join(", "),
        manufacturer: i.manufacturer == null ? "?" : i.manufacturer.fullName,
        os: i.operatingSystem == null ? null : i.operatingSystem.fullVersionedName,
        networks: this.formatNetworks_(i.attachedNetworks),
        localAddrresses: this.formatNetworkAddresses_(i.attachedNetworks),
      });
    });
    return result;
  }
}
</script>

<style lang="scss">
.devices {
  position: relative;
  max-width: 90%;
  margin: auto;
  margin-top: 20px;
}

.deviceList {
  margin-top: 10px;
}

.flip-list-move {
  transition: transform 500ms;
}
</style>
