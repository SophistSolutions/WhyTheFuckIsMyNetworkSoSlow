<template>
  <div class="network">
  <div v-for="network in networks" :key="network.id">
    <img alt="Vue logo" src="../assets/logo.png">
    <h1>Network ID : {{ network.id }}</h1>
  </div>
  </div>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { INetwork } from '@/models/network.ts';

import { fetchNetworks } from '@/proxy/api.ts';
//import HelloWorld from '@/components/HelloWorld.vue'; // @ is an alias to /src

let test : INetwork = {id : "192.168.244.0/24"}



@Component({
    name : "Network"
})
export default class Network extends Vue {
    networks: INetwork[] = [];

    async created() {
      debugger
      // TODO break out network from promise
      
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
