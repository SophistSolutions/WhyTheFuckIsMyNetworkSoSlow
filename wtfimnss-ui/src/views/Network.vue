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
        <p>CIDR: {{ network.networkAddresses[0] }}</p>
        <p>DNS Servers: {{ network.DNSServers }}</p>
        <div id="gateway-network" v-if="isGatewayNetwork(network)">
          <p>Gateways: {{ network.gateways[0] }}</p>
          <p>Public IP: {{ network.externalAddresses[0] }}</p>
          <p>Provider: {{ network.internetServiceProvider.name }}</p>
        </div>
        <div @click="setSelectedNetwork(network.id)">
          <v-btn small color="primary">Select As Primary Network</v-btn>
        </div>
      </v-card-text>
    </v-card>
  </div>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { INetwork } from '@/models/network/INetwork';

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

    private isGatewayNetwork(network: INetwork): boolean {
      return (network.gateways === undefined || network.gateways.length === 0) ? false : true;
    }
}
</script>

<style scoped lang="scss">
.network {
  padding-bottom: 10px;
}
</style>