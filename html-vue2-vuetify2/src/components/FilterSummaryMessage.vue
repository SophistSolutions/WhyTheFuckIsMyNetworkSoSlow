<template>
  <span>{{ msg }}</span>
</template>

<script lang="ts">
import { Component, Prop, Vue, Watch } from "vue-property-decorator";

/*
 *  This is for use in the filter section of the app-bar, to say how much is filtered out.
 */
@Component({
  name: "FilterSummaryMessage",
})
export default class FilterSummaryMessage extends Vue {
  @Prop({ required: true })
  public nTotalItems!: number;

  @Prop({ required: false })
  public filtered!: boolean;

  @Prop({ required: true })
  public nItemsSelected!: number;

  @Prop({ default: "items" })
  public itemsName!: string;

  private msg: string = "";

  private created() {
    this.onChange();
  }

  @Watch("filtered")
  @Watch("nTotalItems")
  @Watch("nItemsSelected")
  @Watch("itemsName")
  private onChange() {
    let filtered = this.filtered;
    if (filtered === null) {
      filtered = this.nItemsSelected === this.nTotalItems;
    }
    if (filtered) {
      this.msg = `Filtered: showing ${this.nItemsSelected} of ${this.nTotalItems} ${this.itemsName}`;
    } else {
      this.msg = `Unfiltered: all ${this.nTotalItems} ${this.itemsName} showing`;
    }
  }
}
</script>

<style scoped lang="scss">
div {
  text-align: right;
}
</style>
