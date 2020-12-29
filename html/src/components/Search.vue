<template>
  <v-btn icon>
    <v-icon>mdi-magnify</v-icon>
    <v-text-field
      v-model="searchFor_bwa"
      append-icon="mdi-magnify"
      label="Search"
      single-line
      hide-details
      @keyup="$emit('update:searchFor', searchFor_bwa)"
    ></v-text-field>
  </v-btn>
</template>

<script lang="ts">
import { Component, Prop, Vue, Watch } from "vue-property-decorator";

@Component({
  name: "Search",
})
export default class AppBar extends Vue {
  @Prop()
  public searchFor: string = "";
  private searchFor_bwa: string = ""; // see https://stackoverflow.com/questions/43933405/can-i-disable-certain-vue-warnings-for-a-specific-component

  @Watch("searchFor")
  watchChangeInSearchFor() {
    this.searchFor_bwa = this.searchFor;
  }
}
</script>

<style scoped lang="scss"></style>
