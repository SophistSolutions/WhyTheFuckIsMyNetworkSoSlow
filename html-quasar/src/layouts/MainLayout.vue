
<script lang="ts">
import { defineComponent, ref } from 'vue';
import EssentialLink from 'components/EssentialLink.vue';

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

export default defineComponent({
  name: 'MainLayout',

  components: {
    EssentialLink
  },

  setup() {
    const leftDrawerOpen = ref(false)

    return {
      essentialLinks: linksList,
      leftDrawerOpen,
      toggleLeftDrawer() {
        leftDrawerOpen.value = !leftDrawerOpen.value
      }
    }
  }
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
          <template v-slot:divider>
            <q-icon name="mdi-chevron-right"/>
          </template>
          <template v-for="(item, index) in this.$route.meta.breadcrumbs" :key="index">
            <q-breadcrumbs-el :href="item.href" :disabled="item.disabled" :label="item.text.toUpperCase()" />
          </template>
        </q-breadcrumbs>

        <q-btn flat dense round icon="mdi-dots-vertical" style="margin-left: 1in" aria-label="Menu" color="white">
          <q-menu>
            <q-list style="min-width: 100px">
              <template v-for="(item, index) in this.$router.options.routes" :key="index">
                <q-item v-if="item.name && item?.meta?.showInDotDotDotMenu" clickable v-close-popup :to="item.path">
                  <q-item-section> {{ item.name }}</q-item-section>
                </q-item>
                <q-separator v-if="item?.meta?.divderAfter" />
              </template>
            </q-list>
          </q-menu>
        </q-btn>

        <template v-slot:extension v-if="this.$slots.extrastuff">
          <slot name="extrastuff" />
        </template>

      </q-toolbar>
    </q-header>

    <q-drawer v-model="leftDrawerOpen" show-if-above bordered>
      <q-list>
        <q-item-label header>
          Essential Links
        </q-item-label>
        <EssentialLink v-for="link in essentialLinks" :key="link.title" v-bind="link" />
      </q-list>
    </q-drawer>

    <q-page-container>
      <router-view />
    </q-page-container>
  </q-layout>
</template>
