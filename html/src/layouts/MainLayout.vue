
<script setup lang="ts">

import { defineComponent, defineProps, Ref, ref } from 'vue';

import { useRoute, useRouter } from 'vue-router'


import EssentialLink from 'components/EssentialLink.vue';


const route = useRoute()
const router = useRouter()

// Leave around stuff from sample layout, for a bit...
const linksList = [
  {
    title: 'Docs',
    caption: 'quasar.dev',
    icon: 'school',
    link: 'https://quasar.dev'
  },
  {
    title: 'Github',
    caption: 'github.com/quasarframework',
    icon: 'code',
    link: 'https://github.com/quasarframework'
  },
  {
    title: 'Discord Chat Channel',
    caption: 'chat.quasar.dev',
    icon: 'chat',
    link: 'https://chat.quasar.dev'
  },
  {
    title: 'Forum',
    caption: 'forum.quasar.dev',
    icon: 'record_voice_over',
    link: 'https://forum.quasar.dev'
  },
  {
    title: 'Twitter',
    caption: '@quasarframework',
    icon: 'rss_feed',
    link: 'https://twitter.quasar.dev'
  },
  {
    title: 'Facebook',
    caption: '@QuasarFramework',
    icon: 'public',
    link: 'https://facebook.quasar.dev'
  },
  {
    title: 'Quasar Awesome',
    caption: 'Community Quasar projects',
    icon: 'favorite',
    link: 'https://awesome.quasar.dev'
  }
];

const leftDrawerOpen = ref(false)

function toggleLeftDrawer() {
  leftDrawerOpen.value = !leftDrawerOpen.value
}

defineComponent({
  components: {
    EssentialLink
  },
});
</script>

<template>
  <q-layout view="lHh Lpr lFf">
    <q-header elevated>
      <q-toolbar class="justify-between">
        <q-btn flat dense round icon="menu" aria-label="Menu" @click="toggleLeftDrawer" />
        <q-toolbar-title>
          WhyTheFuckIsMyNetworkSoSlow
        </q-toolbar-title>
        <q-breadcrumbs separator=">" active-color="secondary">
          <template v-for="(item, index) in route.meta.breadcrumbs" :key="index">
            <q-breadcrumbs-el :href="item.href" :disabled="item.disabled" :label="item.text.toUpperCase()" />
          </template>
        </q-breadcrumbs>
        <q-btn flat dense round icon="mdi-dots-vertical" style="margin-left: 1in" aria-label="Menu" color="white">
          <q-menu>
            <q-list style="min-width: 100px">
              <template v-for="(item, index) in router.options.routes" :key="index">
                <q-item v-if="item.name && item?.meta?.showInDotDotDotMenu" clickable v-close-popup :to="item.path">
                  <q-item-section> {{ item.name }}</q-item-section>
                </q-item>
                <q-separator v-if="item?.meta?.divderAfter" />
              </template>
            </q-list>
          </q-menu>
        </q-btn>
      </q-toolbar>
      <div class="row" id="CHILD_HEADER_SECTION" />
    </q-header>

    <q-drawer v-model="leftDrawerOpen" bordered>
      <q-list>
        <q-item-label header>Essential Links</q-item-label>
        <EssentialLink v-for="link in linksList" :key="link.title" v-bind="link" />
      </q-list>
    </q-drawer>

    <q-page-container>
      <router-view />
    </q-page-container>
  </q-layout>
</template>
