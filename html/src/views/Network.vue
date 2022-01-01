<template>
  <v-container>
    <app-bar />
    <v-card>
      <v-card-title>
        Network {{ network == null ? "loading..." : '"' + GetNetworkName(network) + '"' }}
        <v-spacer></v-spacer>
      </v-card-title>
      <NetworkDetails class="detailsSection" :networkId="this.$route.params.id" v-if="network" />
      <div>aa</div>
    </v-card>
  </v-container>
</template>

<script lang="ts">
import { INetwork } from "@/models/network/INetwork";
import { GetNetworkName } from "@/models/network/Utils";
import { Options, Vue } from 'vue-class-component'

@Options({
  name: "Network",
  components: {
    AppBar: () => import("@/components/AppBar.vue"),
    NetworkDetails: () => import("@/components/NetworkDetails.vue"),
  },
})
export default class Network extends Vue {
  private polling: undefined | number = undefined;

  private GetNetworkName = GetNetworkName;

  public created() {
    this.pollData();
  }

  public beforeDestroy() {
    clearInterval(this.polling);
  }

  private pollData() {
    // this load is just for the name, so little point
    this.$store.dispatch("fetchNetwork", this.$route.params.id);
    this.polling = setInterval(() => {
      this.$store.dispatch("fetchNetwork", this.$route.params.id);
    }, 60 * 1000);
  }

  private get network(): INetwork | undefined {
    return this.$store.getters.getNetwork(this.$route.params.id);
  }
}
</script>

<style lang="scss">
.detailsSection {
  margin-left: 2em;
}
</style>
