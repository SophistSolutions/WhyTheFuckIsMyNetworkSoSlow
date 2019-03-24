<template>
  <div class="networks">
  <div class="networkList" v-for="network in networks" :key="network.id">
    <Network name=Network  :network=network></Network>
  </div>
  </div>
</template>

<script lang="ts">
import { INetwork } from "@/models/network/INetwork";
import { Component, Vue } from "vue-property-decorator";

import { fetchNetworks } from "@/proxy/API";

@Component({
    name : "Networks",
    components : {
      Network: () => import("@/components/Network.vue"),
    },
})
export default class Networks extends Vue {
  // TODO fix so networks doesnt call fetch everytime restarted
    // Trying to replace this with calls to the store instead of directly calling API
    private created() {
      this.$store.dispatch("fetchAvailableNetworks");
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
