<template>
  <div class="networks">
    <h1>Networks</h1>
  <div v-for="network in networks" :key="network.id">
    <Network name=Network  :network=network></Network>
  </div>
  </div>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { INetwork } from '@/models/Network/INetwork';

import { fetchNetworks } from '@/proxy/API';

import Network from '@/views/Network.vue'


@Component({
    name : "Networks",
    components : {
      'Network' : Network
    }
})
export default class Networks extends Vue {
    networks: INetwork[] = [];

    async created() {
      this.networks = await fetchNetworks();
    }

    // created(){

    //   fetchNetworks()
    //     .then((networks: INetwork[]) => {
    //       this.networks = networks;
    //     })
    // }
}
</script>
