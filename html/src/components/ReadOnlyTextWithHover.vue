<template>
<span>
    <span v-if="!link" :title="useTitle">{{ message }}</span>
    <a v-if="link" :href=link :title="useTitle" >{{ message }}</a>
</span>
</template>

<script lang="ts">
import { Options, Vue } from 'vue-class-component'
import { Watch, Prop } from 'vue-property-decorator'

/*
 */
@Options({
  name: 'ReadOnlyTextWithHover'
})
export default class ReadOnlyTextWithHover extends Vue {
    @Prop({
      required: true
    })
    public message!: string;

    @Prop({
      required: false,
      default: null
    })
    public link!: string | null;

    @Prop({
      required: false,
      default: null
    })
    public popupTitle!: string | null;

    private useTitle = '';

    public created () {
        this.onChange();
    }

    @Watch('message')
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
