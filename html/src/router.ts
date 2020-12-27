import Vue from "vue";
import Router from "vue-router";

Vue.use(Router);

export default new Router({
  routes: [
    {
      path: "/",
      name: "Home",
      component: () => import(/* webpackChunkName: "Home" */ "./views/Home.vue"),
      meta: {
        breadcrumbs: [{ text: "Home", exact: true, disabled: true }],
      },
    },
    {
      path: "/about",
      name: "About",
      component: () => import(/* webpackChunkName: "About" */ "./views/About.vue"),
      meta: {
        breadcrumbs: [
          { text: "Home", href: "/", disabled: false, exact: true },
          { text: "About", disabled: true },
        ],
      },
    },
    {
      path: "/devices",
      name: "Devices",
      // route level code-splitting
      // this generates a separate chunk (about.[hash].js) for this route
      // which is lazy-loaded when the route is visited.
      component: () => import(/* webpackChunkName: "Devices" */ "./views/Devices.vue"),
      meta: {
        breadcrumbs: [
          { text: "Home", href: "/" },
          { text: "Devices", disabled: true },
        ],
      },
    },
    {
      path: "/networks",
      name: "Networks",
      // route level code-splitting
      // this generates a separate chunk (about.[hash].js) for this route
      // which is lazy-loaded when the route is visited.
      component: () => import(/* webpackChunkName: "NetworkView" */ "./views/NetworksView.vue"),
      meta: {
        breadcrumbs: [
          { text: "Home", href: "/", exact: true },
          { text: "Networks", disabled: true },
        ],
      },
    },
    {
      path: "/networks-DEPRECATED",
      name: "Networks-DEPRECATED",
      // route level code-splitting
      // this generates a separate chunk (about.[hash].js) for this route
      // which is lazy-loaded when the route is visited.
      component: () =>
        import(
          /* webpackChunkName: "NetworkView-DEPRECATED" */ "./views/NetworkView-DEPRECATED.vue"
        ),
      meta: {
        breadcrumbs: [
          { text: "Home", href: "/", exact: true },
          { text: "Networks {deprecated}", disabled: true },
        ],
      },
    },
    {
      path: "/devices-DEPRECATED",
      name: "Devices {DEPRECATED}",
      // route level code-splitting
      // this generates a separate chunk (about.[hash].js) for this route
      // which is lazy-loaded when the route is visited.
      component: () =>
        import(/* webpackChunkName: "DeviceView-DEPRECATED" */ "./views/DeviceView-DEPRECATED.vue"),
      meta: {
        breadcrumbs: [
          { text: "Home", href: "/", exact: true },
          { text: "Devices{deprecated}", disabled: true },
        ],
      },
    },
  ],
});
