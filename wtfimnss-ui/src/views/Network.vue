<template>
  <div class="network">
    <p>ID: {{ network.id }}</p>
    <p>CIDR: {{ network.networkAddresses }}</p>
  </div>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { INetwork } from '@/models/Network/INetwork';

@Component({
    name : 'Network',
    props : {
      network: Object as () => INetwork,
    },
})
export default class Network extends Vue {

}
</script>
