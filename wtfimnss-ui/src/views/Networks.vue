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
import * as Store from '../store';

import Network from '@/views/Network.vue';


@Component({
    name : 'Networks',
    components : {
      Network,
    },
})
export default class Networks extends Vue {
    private networks: INetwork[] = [];

    // Trying to replace this with calls to the store instead of directly calling API
    private async created() {
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
