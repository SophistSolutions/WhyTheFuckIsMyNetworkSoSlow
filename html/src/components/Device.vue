<template>
  <div class="device">
    <v-card class="device-pill" dark>
      <v-card-title>
        {{ device.name }}
      </v-card-title>

      <v-card-text>
        <p v-if="device.type">Type: {{ device.type }}</p>
        <p v-if="device.operatingSystem">Type: {{ device.operatingSystem }}</p>
        <p v-if="device.presentationURL"><a href="{{ device.presentationURL }}" target="_blank">Open Device</a></p>
        <p>Internet Addresses: {{ device.internetAddresses }}</p>
      </v-card-text>
    </v-card>
  </div>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { IDevice } from '@/models/device/IDevice';

@Component({
    name : 'Device',
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