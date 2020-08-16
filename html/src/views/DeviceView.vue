<template>
  <v-container class="devices">
    <v-card>
      <v-card-title>
        Devices
        <v-spacer></v-spacer>
        <v-text-field
          v-model="search"
          append-icon="mdi-magnify"
          label="Search"
          single-line
          hide-details
        ></v-text-field>
      </v-card-title>
      <v-data-table
        dense
        v-model="selectedRows"
        :headers="headers"
        :items="devices"
        :single-select="true"
        :search="search"
        :sort-by="['name', 'manufacturer']"
        :sort-desc="[false, true]"
        multi-sort
        item-key="id"
        class="elevation-1"
      >
        <template v-slot:top>
          <!-- replace this with search 
          <v-switch v-model="singleSelect" label="Single select" class="pa-3"></v-switch>
          -->
        </template>
        <template v-slot:item.manufacturer="{ item }"
          >{{ item.manufacturer == null ? "?" : item.manufacturer.fullName }}
        </template>
        <template v-slot:expanded-item="{ headers, item }">
          <td :colspan="headers.length">More info about {{ item.name }}</td>
        </template>

        <template v-slot:item="{ item }">
          <tr
            :class="selectedRows.indexOf(item.id) > -1 ? 'yellow' : ''"
            @click="rowClicked($event, item)"
          >
            <td>{{ item.name }}</td>
            <td>{{ item.type == null ? null : item.type.join(", ") }}</td>
            <td>{{ item.manufacturer == null ? null : item.manufacturer.fullName }}</td>
            <td>
              {{ item.operatingSystem == null ? null : item.operatingSystem.fullVersionedName }}
            </td>
            <td>{{ formatNetworks_(item.attachedNetworks) }}</td>
            <td>{{ formatNetworkAddresses_(item.attachedNetworks) }}</td>
          </tr>
        </template>
      </v-data-table>
    </v-card>
  </v-container>
</template>

<script lang="ts">
import { IDevice } from "@/models/device/IDevice";
import { Component, Vue } from "vue-property-decorator";

import { compareValues } from "@/CustomSort";
import { ISortBy, SortFieldEnum } from "@/models/device/SearchSpecification";
import { fetchNetworks } from "@/proxy/API";

@Component({
  name: "Devices",
  components: {
    Device: () => import("@/components/Device-DEPRECATED.vue"),
  },
})
export default class Devices extends Vue {
  private polling: undefined | number = undefined;

  selectedRows = [];

  rowClicked(e, row) {
    this.toggleSelection(row.id);
    // @todo fix proper handling of shift key !e.shiftKey && !
    if (!e.ctrlKey) {
      // single select unless shift key
      this.selectedRows = this.selectedRows.filter((selectedKeyID) => selectedKeyID == row.id);
    }
    console.log(row);
  }
  toggleSelection(keyID) {
    if (this.selectedRows.includes(keyID)) {
      this.selectedRows = this.selectedRows.filter((selectedKeyID) => selectedKeyID !== keyID);
    } else {
      this.selectedRows.push(keyID);
    }
  }

  private formatNetworks_(attachedNetworks: { [key: string]: INetworkAttachmentInfo }): string {
    let addresses = [];
    Object.entries(attachedNetworks).forEach((element) => {
      console.log(element);
      addresses.push(element[0]);
    });
    return addresses.join(", ");
  }

  private formatNetworkAddresses_(attachedNetworks: {
    [key: string]: INetworkAttachmentInfo;
  }): string {
    let addresses = [];
    Object.entries(attachedNetworks).forEach((element) => {
      console.log(element);
      element[1].networkAddresses.forEach((e) => addresses.push(e));
    });
    return addresses.join(", ");
  }

  private search: string = "";

  private get headers(): IDevice[] {
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
        value: "network",
      },
      {
        text: "Local Address",
        value: "attachedNetworks",
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
    }, 10000);
  }

  private get devices(): IDevice[] {
    const devices: IDevice[] = this.$store.getters.getDevices;
    // shallow clone
    const ret = [...devices];
    //return ret.sort();
    return ret;
    // TODO move sort to store

    // return (this.$store.getters.getDevices) ? JSON.parse(JSON.stringify(devices)) : [];
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
