<template>
  <div class="networks">
    <app-bar />
    <div class="networkList" v-for="network in networks" :key="network.id">
      <Network name="Network" :network="network"></Network>
    </div>
  </div>
</template>

<script lang="ts">
import { INetwork } from "@/models/network/INetwork";
import { Options, Vue } from 'vue-class-component'
import { fetchNetworks } from "@/proxy/API";

@Options({
  name: "Networks",
  components: {
    AppBar: () => import("@/components/AppBar.vue"),
    Network: () => import("@/components/Network-DEPRECATED.vue"),
  },
})
export default class Networks extends Vue {
  private polling?: number;

  public created() {
    this.fetchAvailableNetworks();
    this.pollData();
  }

  public beforeDestroy() {
    clearInterval(this.polling);
  }

  private fetchAvailableNetworks() {
    this.$store.dispatch("fetchAvailableNetworks");
  }

  private pollData() {
    this.polling = setInterval(() => {
      this.fetchAvailableNetworks();
    }, 10000);
  }

  private get networks(): INetwork[] {
    return this.$store.getters.getAvailableNetworks;
  }
}
</script>

<style lang="scss">
.networks {
  position: relative;
  max-width: 70%;
  margin: auto;
  margin-top: 20px;
}
.networkList {
  margin-top: 10px;
}
</style>
