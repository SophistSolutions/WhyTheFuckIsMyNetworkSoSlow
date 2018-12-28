<template>
  <div class="network">
    <v-card dark>
      <v-card-title>
        {{ network.id }}
      </v-card-title>

      <v-card-text>
        <p>CIDR: {{ network.networkAddresses }}</p>
      </v-card-text>
    </v-card>
  </div>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { INetwork } from '@/models/Network/INetwork';

@Component({
    name : 'Network',
    props : {
      network: Object as () => INetwork,
    },
})
export default class Network extends Vue {

}
</script>
