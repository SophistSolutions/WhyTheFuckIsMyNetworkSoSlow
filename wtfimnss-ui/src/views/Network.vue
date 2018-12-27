<template>
  <div class="network">
    <h1>Networks</h1>
  <div v-for="network in networks" :key="network.id">
    <h3>ID: {{ network.id }}</h3>
    <h3>CIDR: {{ network.networkAddresses }}</h3>
  </div>
  </div>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { INetwork } from '@/models/Network/INetwork';

import { fetchNetworks } from '@/proxy/API';


@Component({
    name : "Network"
})
export default class Network extends Vue {
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
