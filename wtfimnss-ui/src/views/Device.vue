<template>
  <div class="device">
    <v-card dark>
      <v-card-title>
        {{ device.name }}
      </v-card-title>

      <v-card-text>
        <p>Type: {{ device.type }}</p>
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
.device {
  padding-bottom: 10px;
}
</style>