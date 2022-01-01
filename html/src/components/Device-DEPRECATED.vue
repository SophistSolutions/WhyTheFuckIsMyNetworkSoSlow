<template>
  <div class="device">
    <v-card class="device-pill" dark>
      <v-card-title>{{ device.name }}</v-card-title>

      <v-card-text>
        <p v-if="device.type">Type: {{ device.type.join(", ") }}</p>
        <p v-if="device.operatingSystem">OS: {{ device.operatingSystem.fullVersionedName }}</p>
        <p v-if="device.presentationURL">
          <a :href="device.presentationURL" target="_blank">Open Device</a>
        </p>
        <p v-if="device.icon">
          <img :src="device.icon" />
        </p>
        <p v-if="device.manufacturer">
          Manufacturer:
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
        </p>
        <p>Internet Addresses: {{ getDeviceNetworkAddresses_(device) }}</p>
        <p>Hardware Addresses: {{ getDeviceHardwareAddresses_(device) }}</p>
      </v-card-text>
    </v-card>
  </div>
</template>

<script lang="ts">
import { Options, Vue } from 'vue-class-component'
import { IDevice, INetworkAttachmentInfo } from "../models/device/IDevice";

@Options({
  name: "Device",
  props: {
    device: Object as () => IDevice,
  },

  methods: {
    getDeviceNetworkAddresses_(d: IDevice) {
      let result = "";
      for (const value of Object.entries(d.attachedNetworks)) {
        if (result !== "") {
          result += ", ";
        }
        result += value[1].localAddresses.join(", ");
      }
      return result;
    },
    getDeviceHardwareAddresses_(d: IDevice) {
      let result = "";
      for (const value of Object.entries(d.attachedNetworks)) {
        if (result !== "") {
          result += ", ";
        }
        result += value[1].hardwareAddresses.join(", ");
      }
      return result;
    },
  },
})
export default class Device extends Vue {}
</script>

<style scoped lang="scss">
.device-pill {
  padding-bottom: 10px;
  border-radius: 25px;
}
v-card-title {
  position: relative;
  height: 40px;
}
</style>
