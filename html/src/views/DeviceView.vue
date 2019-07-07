<template>
  <v-container class="devices">
    <v-layout row wrap>
      <v-flex md12>
        <div>
          Sort by:
        </div>
        <div>
          <v-btn
            color="blue-grey"
            class="white--text"
            @click="selectSortField(SortFieldEnum.ADDRESS)"
          > Address
            <v-icon medium dark right>device_hub</v-icon>
          </v-btn>
          <v-btn
            color="blue-grey"
            class="white--text"
            @click="selectSortField(SortFieldEnum.PRIORITY)"
          > Relavence
            <v-icon medium dark right>priority_high</v-icon>
          </v-btn>
          <v-btn
            color="blue-grey"
            class="white--text"
            @click="selectSortField(SortFieldEnum.NAME)"
          > Name
            <v-icon medium dark right>perm_identity</v-icon>
          </v-btn>
          <v-btn
            color="blue-grey"
            class="white--text"
            @click="selectSortField(SortFieldEnum.TYPE)"
          > Type
            <v-icon medium dark right>category</v-icon>
          </v-btn>
        </div>
      </v-flex>
      <v-flex md12 class="deviceList" v-for="device in sortedDevices" :key="device.id">
        <Device name="Device" :device="device"></Device>
      </v-flex>
    </v-layout>
  </v-container>
</template>

<script lang="ts">
import { IDevice } from "@/models/device/IDevice";
import { Component, Vue } from "vue-property-decorator";

import { compareValues } from "@/CustomSort";
import { fetchNetworks } from "@/proxy/API";
import { SortFieldEnum } from "@/models/device/SearchSpecification";

@Component({
  name: "Devices",
  components: {
    Device: () => import("@/components/Device.vue"),
  }
})
export default class Devices extends Vue {
  private polling: undefined | number = undefined;

  private sortField: undefined | SortFieldEnum = undefined;
  private sortOrder: string = "asc";

  // TODO fix hack to expose enum in template
  private SortFieldEnum: typeof SortFieldEnum = SortFieldEnum;

  private get sortedDevices(): IDevice[] {
    // @todo not sure the right way to handle this? We want to 'store' the mapping of ID to device data, and
    // we probably want a separate deviceid (sorted) list in the store, and have it depend on the sort order (and filter) objects.
    let deviceArray: IDevice[] = this.devices;
    return deviceArray;
    //return deviceArray.sort(compareValues(this.sortField, this.sortOrder));
  }

  private created() {
    this.fetchDevices();
    this.pollData();
  }

  private beforeDestroy() {
    clearInterval(this.polling);
  }

  private fetchDevices() {
    const sortFields: SortFieldEnum[] | undefined = (this.sortField) ? [this.sortField] : undefined;
    this.$store.dispatch("fetchDevices", sortFields);
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
    return ret.sort();

    // TODO move sort to store

    //return (this.$store.getters.getDevices) ? JSON.parse(JSON.stringify(devices)) : [];
  }

  private selectSortField(sortField: SortFieldEnum): void {
    this.sortField = sortField;
    this.$store.dispatch("fetchDevices", [sortField]);
  }
}
</script>

<style lang="scss">
.devices {
  position: relative;
  max-width: 70%;
  margin: auto;
  margin-top: 20px;
}
.deviceList {
  margin-top: 10px;
}
</style>