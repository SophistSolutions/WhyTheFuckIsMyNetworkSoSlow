<template>
<a :href="link" target="_blank"
              ><img src="images/as-new-page-link.png" title="View in new page" width=19 style="padding-top: 5px; float:right"
            /></a>
</template>

<script lang="ts">
import {
    Component,
    Prop,
    Vue,
    Watch
} from "vue-property-decorator";

/*
 */
@Component({
    name: "Link2DetailsPage",
})
export default class Link2DetailsPage extends Vue {
    @Prop({
        required: false,
        default: null
    })
    public link!: string | null;
}
</script>

<style scoped lang="scss"></style>
</template>
