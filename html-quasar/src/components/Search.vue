<script setup lang="ts">
import { defineProps, defineEmits, watch, ref, Ref } from 'vue';

const props = defineProps({
  searchFor: { type: String, required: false },
})

const emit = defineEmits(['update:searchFor'])

var search: Ref<string|undefined> = ref(props.searchFor);

const updateSearchFor = (newValue:string|number|null) => {
    emit('update:searchFor', newValue)
}
</script>

<template>
  <q-icon name="mdi-magnify" :left="true" />
  <q-input :modelValue="search" label="Search"  @update:modelValue="updateSearchFor"  />
</template>
