<template>

  <div class="param">

    <input type="text" size="30" v-model="UserInput" />

    <input type="button" value="Go to route" @click="GoToRoute()">

  </div>

</template>

<script>
    export default {
        name: 'Param',
        data () {
            return {
            UserInput :''
            }

        },
        methods:{
            GoToRoute : function(){
            this.$router.push({ name: 'ParamDetails', params: { id: this.UserInput }})
            }
        }
    }
</script>
