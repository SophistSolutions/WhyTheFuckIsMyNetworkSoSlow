import Vue from 'vue'
import Router from 'vue-router'
import NetworkVisualizer from '@/components/NetworkVisualizer'

Vue.use(Router)

export default new Router({
  routes: [
    {
      path: '/',
      name: 'NetworkVisualizer',
      component: NetworkVisualizer
    }
  ]
})
