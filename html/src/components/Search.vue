<template>
  <v-text-field
    v-model="searchForBWA"
    append-icon="mdi-magnify"
    label="Search"
    single-line
    hide-details
    @keyup="$emit('update:searchFor', searchForBWA)"
  ></v-text-field>
</template>

<script lang="ts">
import { Component, Prop, Vue, Watch } from "vue-property-decorator";

@Component({
  name: "Search",
})
export default class AppBar extends Vue {
  @Prop()
  public searchFor: string = "";

  // see https://stackoverflow.com/questions/43933405/can-i-disable-certain-vue-warnings-for-a-specific-component
  private searchForBWA: string = "";

  @Watch("searchFor")
  private watchChangeInSearchFor() {
    this.searchForBWA = this.searchFor;
  }
}
</script>

<style scoped lang="scss"></style>
