<template>
  <hello-world />
</template>

<script lang="ts">
import Vue from "vue";
import HelloWorld from "../components/HelloWorld.vue";

export default Vue.extend({
  name: "Home",

  components: {
    HelloWorld,
  },

  mounted() {
    // @todo fix hack - dont do in mounted, use https://medium.com/@fagnersaraujo/automated-breadcrumbs-with-vuejs-7e1051de8028
    // to get from router/watch router
    this.$root.$children[0].$data.breadcrumbs = [
      { text: "Home", disabled: false, to: "/" },
      { text: "faketesthome", disabled: false, to: "hello" },
    ];
  },
});
</script>
