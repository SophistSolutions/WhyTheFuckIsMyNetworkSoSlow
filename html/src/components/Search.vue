<script setup lang="ts">
import { defineProps, defineEmits, ref, Ref } from 'vue';

const props = defineProps({
  searchFor: { type: String, required: false },
})

const emit = defineEmits(['update:searchFor'])

var search: Ref<string | undefined> = ref(props.searchFor);

const updateSearchFor = (newValue: string | number | null) => {
  emit('update:searchFor', newValue)
}
</script>

<template>
  <div class="row no-wrap items-baseline">
    <q-icon name="mdi-magnify" :left="true" />
    <q-input dense dark v-model="search" label="Search" @update:modelValue="updateSearchFor" />
  </div>
</template>
