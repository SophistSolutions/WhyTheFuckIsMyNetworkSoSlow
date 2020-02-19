import Vue from "vue";
import Router from "vue-router";

Vue.use(Router);

export default new Router({
  routes: [
    {
      path: "/",
      name: "home",
      redirect: "networks-DEPRECATED",
    },
    {
      path: "/about",
      name: "about",
      component: () => import(/* webpackChunkName: "About" */ "./views/About.vue"),
    },
    {
      path: "/hello",
      name: "hello",
      component: () => import(/* webpackChunkName: "About" */ "./views/Home.vue"),
    },
    {
      path: "/networks-DEPRECATED",
      name: "Networks {DEPRECATED}",
      // route level code-splitting
      // this generates a separate chunk (about.[hash].js) for this route
      // which is lazy-loaded when the route is visited.
      component: () =>
        import(/* webpackChunkName: "NetworkView-DEPRECATED" */ "./views/NetworkView-DEPRECATED.vue"),
    },
    {
      path: "/devices-DEPRECATED",
      name: "Devices {DEPRECATED}",
      // route level code-splitting
      // this generates a separate chunk (about.[hash].js) for this route
      // which is lazy-loaded when the route is visited.
      component: () =>
        import(/* webpackChunkName: "DeviceView-DEPRECATED" */ "./views/DeviceView-DEPRECATED.vue"),
    },
  ],
});
