<template>
  <div class="device">
    <v-card class="device-pill" dark>
      <v-card-title>
        {{ device.name }}
      </v-card-title>

      <v-card-text>
        <p v-if="device.type">Type: {{ device.type.join(", ") }}</p>
        <p v-if="device.operatingSystem">OS: {{ device.operatingSystem.fullVersionedName }}</p>
        <p v-if="device.presentationURL"><a :href="device.presentationURL" target="_blank">Open Device</a></p>
        <p>Internet Addresses: {{ device.internetAddresses.join(", ") }}</p>
      </v-card-text>
    </v-card>
  </div>
</template>

<script lang="ts">
import { IDevice } from "@/models/device/IDevice";
import { Component, Vue } from "vue-property-decorator";

@Component({
    name : "Device",
    props : {
      device: Object as () => IDevice,
    },
    methods : {
    },
})
export default class Device extends Vue {

}
</script>

<style scoped lang="scss">
.device-pill {
  padding-bottom: 10px;
  border-radius:25px;
}
v-card-title {
  position: relative;
  height: 40px;
}
</style>