<template>
  <v-container>
    <v-row class="text-center">
      <v-col class="mb-4">
        <h1 class="display-2 font-weight-bold mb-3">
          Why The Fuck is My Network So Slow?
        </h1>
      </v-col>
    </v-row>
    <v-row>
      <v-col class="mb-4">
        Networks (active + favorites)
        <ul>
          <li>Net 1</li>
          <div>: 23 devices, operating normally</div>
          <li>Net 2</li>
          <div>: 23 devices, operating normally</div>
        </ul>
      </v-col>
    </v-row>
  </v-container>
</template>

<script lang="ts">
import Vue from "vue";
import HelloWorld from "../components/HelloWorld.vue";

export default Vue.extend({
  name: "Home",

  components: {
    HelloWorld,
  },
});
</script>
