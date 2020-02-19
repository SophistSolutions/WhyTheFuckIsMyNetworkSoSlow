<template>
  <v-app>
    <v-app-bar app color="primary" dark>
      <div class="d-flex align-center">
        <div>WhyTheFuckIsMyNetworkSoSlow</div>
      </div>

      <v-spacer></v-spacer>

      <div>PUT BREADCRUMBS HERE</div>
      <!-- 
    <a
        v-for="(name, link) in breadcrumbs"
        :key="link"
        :href="eco.href"
        class="subheading mx-3"
        target="_blank"
      >
        {{ eco.text }}
      </a>


      <v-btn href="#networks-DEPRECATED" target="_blank" text>
        <span class="mr-2">networks-DEPRECATED</span>
        <v-icon>mdi-open-in-new</v-icon>
      </v-btn>

 -->
      <v-spacer></v-spacer>

      <v-menu bottom left>
        <template v-slot:activator="{ on }">
          <v-btn icon color="black" v-on="on">
            <v-icon>mdi-dots-vertical</v-icon>
          </v-btn>
        </template>

        <v-list>
          <v-list-item>
            <v-btn href="#about" target="_self" text>
              <span class="mr-2">about</span>
            </v-btn>
          </v-list-item>
          <v-spacer></v-spacer>
          <v-list-item>
            <v-btn href="#devices-DEPRECATED" target="_self" text>
              <span class="mr-2">devices-DEPRECATED</span>
            </v-btn>
          </v-list-item>
          <v-list-item>
            <v-btn href="#networks-DEPRECATED" target="_self" text>
              <span class="mr-2">networks-DEPRECATED</span>
            </v-btn>
          </v-list-item>
          <v-spacer></v-spacer>
          <v-list-item>
            <v-btn href="#hello" target="_blank" text>
              <span class="mr-2">hello</span>
            </v-btn>
          </v-list-item>
        </v-list>
      </v-menu>
    </v-app-bar>

    <v-content>
      <router-view />
    </v-content>
  </v-app>
</template>

<script lang="ts">
import Vue from "vue";

export default Vue.extend({
  name: "App",

  data: () => ({
    breadcrumbs: [{ name: "about", link: "#about" }],
  }),
});
</script>
