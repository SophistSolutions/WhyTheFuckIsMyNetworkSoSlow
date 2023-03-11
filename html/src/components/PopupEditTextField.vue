<script setup lang="ts">
import {
  ref,
  reactive,
  watchEffect,
  toRef, watch
} from "vue";

const props = defineProps({
  // This is the value - if initialValue/newSetValue===null, that will be used; this is just used for display so no need to set to null sometimes
  defaultValue: { type: String, required: true },
  // This is the value of the field being edited. If null, means using default value, and if not-null, must be valid string
  initialValue: { type: String, required: false },
  validator: { type: Function, required: false }
})

const emit = defineEmits(['update:userSetValue'])

// Call this not notify parent component of an edit/change
const updateEditValue = (newValue: string | null) => {
  emit('update:userSetValue', newValue)
}

let userSettingsNetworkName: {
  defaultValue: string,
  initialValue: string | undefined,
  // if newSetUserValue undefined, it hasn't been set by user yet. If ===  null, means CLEARED TO DEFAULT
  newSetUserValue: string | undefined | null
  // Like newSetUserValue, but live updated and not pushed to actual value until right time
  newUserSetValueUI: string | undefined | null
} = reactive({ defaultValue: props.defaultValue, initialValue: undefined, newSetUserValue: undefined, newUserSetValueUI:undefined });

// Forward props changes to reactive userSettingsNetworkName we use in component
watch(toRef(props, 'defaultValue'), () => userSettingsNetworkName.defaultValue = props.defaultValue || "");
watch(toRef(props, 'initialValue'), () => userSettingsNetworkName.initialValue = props.initialValue || "");


// Pointer to DOM field, to use internally in select-all UI flourish
let userSettingsNetworkNameField = ref(null)


function validateNetworkName(v: any) {
  // console.log('enter validateNetworkName v=', v, v && v.length >= 1)
  if (props.validator) {
    return props.validator(v);
  }
  return true;
}

watchEffect(
  () => {
    if (userSettingsNetworkName.newSetUserValue === undefined) {  // never set
      userSettingsNetworkName.newUserSetValueUI = userSettingsNetworkName.initialValue;
      // console.log('CHANGE: since newSetUserValue=undefined, SETTING newUserSetValueUI=', userSettingsNetworkName.newUserSetValueUI)
    }
    else {
      userSettingsNetworkName.newUserSetValueUI = userSettingsNetworkName.newSetUserValue;  // last value ACTUALLY set (not canceled)
      // console.log('CHANGE: since newSetUserValue DEFINED, SETTING newUserSetValueUI=', userSettingsNetworkName.newUserSetValueUI)
    }
  }
)

function updateNetworkName(event: any, scope: any, newValue: string | null) {
  // console.log('Enter updateNetworkName newSetUserValue BEING SET TO=', newValue)
  userSettingsNetworkName.newSetUserValue = newValue;
  scope.value = newValue;
  scope.set();
}
</script>

<template>
  <q-popup-edit v-model="userSettingsNetworkName.newUserSetValueUI" v-slot="scope" @hide="validateNetworkName"
    :validate="validateNetworkName" @update:modelValue="updateEditValue">
    <q-input ref="userSettingsNetworkNameField" autofocus dense v-model="scope.value"
      :hint="`Use Network Name (${userSettingsNetworkName.newUserSetValueUI == null ? 'using ' : ''}default: ${userSettingsNetworkName.defaultValue})`"
      :placeholder="userSettingsNetworkName.defaultValue" :rules="[
        val => scope.validate(val) || 'More than 1 chars required'
      ]" @focus="this.$refs?.userSettingsNetworkNameField.select()">
      <template v-slot:after>
        <q-btn flat dense color="black" icon="cancel" @click.stop.prevent="scope.cancel" title="Make no changes" />
        <q-btn flat dense color="positive" icon="check_circle"
          @click.stop.prevent="updateNetworkName($event, scope, scope.value)"
          :disable="scope.validate(scope.value) === false || scope.value == null"
          title="Use this as (override) Network Name" />
        <q-btn flat dense color="negative" icon="delete" title="Clear override: use default"
          @click.stop.prevent="updateNetworkName($event, scope, null)" />
      </template>
    </q-input>
  </q-popup-edit>
</template>
