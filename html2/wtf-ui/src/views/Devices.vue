
<script setup lang="ts">
import { defineComponent , onMounted} from 'vue';
import { useStore } from 'vuex'
import { duration } from 'moment';
import prettyBytes from 'pretty-bytes';

import { IAbout } from "@/models/IAbout";

import AppBar from "@/components/AppBar.vue";

var polling: undefined | number = undefined;

const store = useStore()

function about(): IAbout {
  return store.getters.getAboutInfo;
}

function pollData() {
  // first time check quickly, then more gradually
  store.dispatch("fetchAboutInfo");
  if (polling) {
    clearInterval(polling);
  }
  polling = setInterval(() => {
    store.dispatch("fetchAboutInfo");
  }, 10 * 1000);
}

onMounted(() => {
  pollData()
})
</script>

<template>
  <div class="about">
    <app-bar />

    <v-container class="pa-6" fluid v-if="about">
  
      </v-container>
  </div>
</template>
