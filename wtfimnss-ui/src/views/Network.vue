<template>
  <div class="network">
    <v-card dark>
      <v-card-title>
        {{ network.id }}
        <v-btn flat icon color="yellow" v-if="selectedNetworkId === network.id">
          <v-icon>star</v-icon>
        </v-btn>
      </v-card-title>

      <v-card-text>
        <p>CIDR: {{ network.networkAddresses }}</p>
        <p>Gateways: {{ network.gateways }}</p>
        <p>PublicIP: {{ network.externalAddresses }}</p>
        <p>Provider: {{ network.internetServiceProvider }}</p>
        <div @click="setSelectedNetwork(network.id)">
          <v-btn small color="primary">Select</v-btn>
        </div>
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
    methods : {
      setSelectedNetwork(networkId) {
        this.$store.dispatch('setSelectedNetwork', networkId);
      },
    },
})
export default class Network extends Vue {
    private get selectedNetworkId(): string {
      return this.$store.getters.getSelectedNetworkId;
    }
}
</script>
