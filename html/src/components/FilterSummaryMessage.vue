<template>
  <div>{{ msg }}</div>
</template>

<script lang="ts">
import { Component, Prop, Vue, Watch } from "vue-property-decorator";

@Component({
  name: "FilterSummaryMessage",
})
export default class FilterSummaryMessage extends Vue {
  @Prop({ required: true })
  public nTotalItems: number | null;

  @Prop({ required: true })
  public nItemsSelected: number | null;

  @Prop({ default: "items" })
  public itemsName: string | null;

  private msg: string = "";

  private created() {
    this.onChange();
  }

  @Watch("nTotalItems")
  @Watch("nItemsSelected")
  @Watch("itemsName")
  private onChange() {
    if (this.nItemsSelected === this.nTotalItems) {
      this.msg = `Unfiltered: all ${this.nTotalItems} ${this.itemsName} showing`;
    } else {
      this.msg = `Filtered: showing ${this.nItemsSelected} of ${this.nTotalItems} ${this.itemsName}`;
    }
  }
}
</script>

<style scoped lang="scss">
div {
  text-align: right;
}
</style>
