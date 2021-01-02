<template>
  <v-btn class="mx-4" fab dark x-small v-on:click="myclick">
    <v-icon dark x-small>
      mdi-close
    </v-icon>
  </v-btn>
</template>
<script lang="ts">
import { Component, Prop, Vue, Watch } from "vue-property-decorator";

/*
 */
@Component({
  name: "ClearButton",
})
export default class ClearButton extends Vue {
  @Prop()
  public click: Function;

  private myclick() {
    this.$emit("click");
  }
}
</script>

<style scoped lang="scss">
v-btn {
  float: right;
}
</style>
