<template>
  <div class="network">
    <v-card dark class="network-pill">
      <v-card-title>

        <v-icon id="selected-star" color="yellow" v-show="selectedNetworkId === network.id">star</v-icon>

        <!-- TODO move to a settings store -->
        <p v-if="debugMode">{{ network.id }}</p>
      </v-card-title>

      <v-card-text>
        <p>CIDR: {{ network.networkAddresses[0] }}</p>
        <p>DNS Servers: {{ network.DNSServers }}</p>
        <div id="gateway-network" v-if="isGatewayNetwork(network)">
          <p>Gateways: {{ network.gateways[0] }}</p>
          <p>Public IP: {{ network.externalAddresses[0] }}</p>
          <p>Internet Service Provider: {{ network.internetServiceProvider.name }}</p>
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
import { DEBUG_MODE } from '@/config';

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
    computed: {
      debugMode(): boolean {
        return DEBUG_MODE;
      }
    }
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
#selected-star {
  position: absolute;
  top: 30px;
  left: 40px;
}
.network-pill {
  padding-bottom: 10px;
  border-radius:25px;
}
v-card-title {
  position: relative;
  height: 40px;
}
</style>