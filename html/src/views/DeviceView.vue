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
          <v-btn
            color="blue-grey"
            class="white--text"
            @click="selectSortAscending(true)"
          >
            <v-icon medium dark>arrow_upward</v-icon>
          </v-btn>
          <v-btn
            color="blue-grey"
            class="white--text"
            @click="selectSortAscending(false)"
          >
            <v-icon medium dark>arrow_downward</v-icon>
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
import { SortFieldEnum, ISortBy } from "@/models/device/SearchSpecification";

@Component({
  name: "Devices",
  components: {
    Device: () => import("@/components/Device.vue"),
  }
})
export default class Devices extends Vue {
  private polling: undefined | number = undefined;

  private sortField: undefined | SortFieldEnum = undefined;
  private sortOrder: undefined | boolean = true;

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
    this.$store.dispatch("fetchDevices", this.getSearchCriteria());
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
    this.fetchDevices();
  }
  private selectSortAscending(sortOrder: boolean): void {
    this.sortOrder = sortOrder;
    this.fetchDevices();
  }
  private getSearchCriteria(): ISortBy | undefined {
    if (!this.sortField) {
      return undefined;
    }

    const searchQuery: ISortBy = { by: this.sortField };

    if (this.sortOrder !== undefined) {
      searchQuery.ascending = this.sortOrder;
    }

    return searchQuery;
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