<template>

  <div class="param">

    <input type="text" size="30" v-model="UserInput" />

    <input type="button" value="Add param" @click="AddParam()">
    <input type="button" value="Delete param" @click="RemoveParam()">

  </div>

</template>

<script>
  import { store } from '../store.js'

    export default {
        name: 'Param',
        data () {
            return {
            UserInput :'',
            storeState: store.state
            }
        },
        methods:{
            AddParam : function(){
            store.addValue(this.UserInput)
            this.$router.push({ name: 'ParamDetails', params: { id: this.UserInput, values: this.storeState.values }})
          },
          RemoveParam : function(){
          store.removeValue(this.UserInput)
          this.$router.push({ name: 'ParamDetails', params: { id: this.UserInput, values: this.storeState.values }})
          }
        }
    }
</script>
