<template>
  <div class="network-visualizer">
    <h1>{{ title }}</h1>
  </div>
</template>

<script>
export default {
  name: 'network-visualizer',
  data () {
    return {
      title: 'Network Visualizer'
    }
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>

  h1 {
    color: #369;
    font-family: Arial, Helvetica, sans-serif;
    font-size: 250%;
  }

  body {
    padding: 0px 0px 0px 0px;
    margin: 0px 0px 0px 0px;
  }

  #deviceInformation {
    min-height: 120px;
    height: 20vh;
  }

  .well {
    margin-bottom:0px;
  }

  .row {
    margin-bottom: 0px;
  }

  .rounded {
    border-radius: 15px;
  }

  .unround {
    border-radius: 0px;
  }


</style>
