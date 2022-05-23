
<script setup lang="ts">
function log(item) {
    console.log(item)
}
</script>

<template>
    <v-app-bar app color="primary" dark>
        <v-app-bar-nav-icon />
        <div class="d-flex align-center">
            <div>WhyTheFuckIsMyNetworkSoSlow</div>
        </div>

        <v-spacer></v-spacer>

        <v-breadcrumbs :items="this.$route.meta.breadcrumbs">
            <template v-slot:divider>
                <v-icon>mdi-chevron-right</v-icon>
            </template>
            <template v-slot:item="{ item }">
                <v-breadcrumbs-item :href="item.href" :disabled="item.disabled">
                    {{ item.text.toUpperCase() }}
                </v-breadcrumbs-item>
            </template>
        </v-breadcrumbs>

        <v-spacer></v-spacer>

        <v-menu open-on-hover>
            <template v-slot:activator="{ props }">
                <v-btn icon color="white" v-bind="props">
                    <v-icon>mdi-dots-vertical</v-icon>
                </v-btn>
            </template>
            <v-list>
                <template v-for="(item,index) in this.$router.options.routes" :key="index">
                    <v-list-item :to="item.path" class="mr-2">
                        {{ item.name }}
                    </v-list-item>
                    <v-divider v-if="item?.meta?.divderAfter"> </v-divider>
                    <v-spacer v-if="item?.meta?.divderAfter"></v-spacer>
                </template>
                <v-divider v-if="this.$slots.extrastuff"> </v-divider>
                <v-spacer v-if="this.$slots.extrastuff"></v-spacer>
            </v-list>
        </v-menu>
        <template v-slot:extension v-if="this.$slots.extrastuff">
            <slot name="extrastuff" />
        </template>
    </v-app-bar>
</template>