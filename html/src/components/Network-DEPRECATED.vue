<template>
  <div class="network">
    <v-card dark class="network-pill">
      <v-card-title>
        <v-icon id="selected-star" color="yellow" v-show="selectedNetworkId === network.id"
          >star</v-icon
        >

        <!-- TODO move to a settings store -->
        <p v-if="debugMode">{{ network.id }}</p>
      </v-card-title>

      <v-card-text>
        <p>CIDR: {{ network.networkAddresses.join(", ") }}</p>
        <p>DNS Servers: {{ network.DNSServers.join(", ") }}</p>
        <div id="gateway-network" v-if="isGatewayNetwork(network)">
          <p>Gateways: {{ network.gateways.join(", ") }}</p>
          <p>Public IP: {{ network.externalAddresses.join(", ") }}</p>
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
import { DEBUG_MODE } from "@/config";
import { INetwork } from "@/models/network/INetwork";
import { Component, Vue } from "vue-property-decorator";

@Component({
  name: "Network",
  props: {
    network: Object as () => INetwork,
  },
  methods: {
    setSelectedNetwork(networkId) {
      this.$store.dispatch("setSelectedNetwork", networkId);
    },
  },
  computed: {
    debugMode(): boolean {
      return DEBUG_MODE;
    },
  },
})
export default class Network extends Vue {
  private get selectedNetworkId(): string {
    return this.$store.getters.getSelectedNetworkId;
  }

  private isGatewayNetwork(network: INetwork): boolean {
    return network.gateways === undefined || network.gateways.length === 0 ? false : true;
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
  border-radius: 25px;
}
v-card-title {
  position: relative;
  height: 40px;
}
</style>
