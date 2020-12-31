<template>
  <span :title="useTitle">{{ message }}</span>
</template>

<script lang="ts">
import { Component, Prop, Vue, Watch } from "vue-property-decorator";

/*
 */
@Component({
  name: "ReadOnlyTextWithTitle",
})
export default class ReadOnlyTextWithTitle extends Vue {
  @Prop({ required: true })
  public message!: string;
  @Prop({ required: false, default: null })
  public popupTitle!: string | null;

  private useTitle: string = "";

  private created() {
    this.onChange();
  }

  @Watch("message")
  @Watch("popupTitle")
  private onChange() {
    this.useTitle = this.popupTitle == null ? this.message : this.popupTitle;
    if (this.useTitle === null) {
      this.useTitle = "";
    }
  }
}
</script>

<style scoped lang="scss"></style>
