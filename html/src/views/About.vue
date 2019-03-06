<template>
  <div class="about">
    <h1>About Why The Fuck is My Network So Slow</h1>
    <div>my version# {{ about.applicationVersion }}</div>
  </div>
</template>


<script lang="ts">
import Vue from 'vue'
import Component from 'vue-class-component'
import { IAbout } from '@/models/IAbout'

@Component({
  props: {
  },
})
export default class About extends Vue {

  constructor() {
    super();
  }
    private created() {
      this.$store.dispatch('fetchAboutInfo')
    }

    private get about(): IAbout {
      return this.$store.getters.getAboutInfo;
    }

}
</script>
