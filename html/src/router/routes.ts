import { RouteRecordRaw } from 'vue-router';

const routes: RouteRecordRaw[] = [
  {
    path: '/',
    name: 'Home',
    component: () => import('layouts/MainLayout.vue'),
    children: [{ path: '', component: () => import('pages/HomePage.vue') }],
    meta: {
      breadcrumbs: [{ text: "Home", exact: true, disabled: true,  }],
      divderAfter: true,
      showInDotDotDotMenu: true
    },
  },
  {
    path: '/sample',
    name: 'Sample',
    component: () => import('layouts/MainLayout.vue'),
    children: [{ path: '', component: () => import('pages/IndexPage.vue') }],
  },
  {
    path: '/about',
    name: 'About',
    component: () => import('layouts/MainLayout.vue'),
    children: [{ path: '', component: () => import('pages/AboutPage.vue') }],
    meta: {
      breadcrumbs: [
        { text: "Home", href: "/#/" },
        { text: "About", disabled: true },
      ],
      showInDotDotDotMenu: true
    },
},




  {
    path: "/device/:id",
    name: "Device",
    component: () => import('layouts/MainLayout.vue'),
    children: [{ path: '', component: () => import('pages/DevicePage.vue') }],
    meta: {
      breadcrumbs: [
        { text: "Home", href: "/#/" },
        { text: "Devices", href: "/#/devices" },
        { text: "Device", disabled: true },
      ],
    },
  },
  {
    path: "/devices",
    name: "Devices",
    component: () => import('layouts/MainLayout.vue'),
    children: [{ path: '', component: () => import('pages/DevicesPage.vue') }],
    meta: {
      breadcrumbs: [
        { text: "Home", href: "/#/" },
        { text: "Devices", disabled: true },
      ],
      showInDotDotDotMenu: true
    },
  },
  //   // @see https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/14
  //   // Cannot get this working but got basically same thing working from created
  //   // method
  //   // beforeEnter: (to, from, next) => {
  //   //   // do something with to.query and then save it in store
  //   //   if (to.query.selectedNetwork) {
  //   //     next({
  //   //       path: "/devices",
  //   //       replace: true,
  //   //       params: { selectedNetwork: to.query.selectedNetwork },
  //   //     });
  //   //   } else {
  //   //     next();
  //   //   }
  //   // },
  //   // props: (route) => ({ selectedNetwork: route.query.selectedNetwork }),
  // },
  {
    path: "/network/:id",
    name: "Network",
    component: () => import('layouts/MainLayout.vue'),
    children: [{ path: '', component: () => import('pages/NetworkPage.vue') }],
    meta: {
      breadcrumbs: [
        { text: "Home", href: "/#/" },
        { text: "Networks", href: "/#/networks" },
        { text: "Network", disabled: true },
      ],
    },
  },
  {
    path: "/networks",
    name: "Networks",
    component: () => import('layouts/MainLayout.vue'),
    children: [{ path: '', component: () => import('pages/NetworksPage.vue') }],
    meta: {
      breadcrumbs: [
        { text: "Home", href: "/#/" },
        { text: "Networks", disabled: true },
      ],
      showInDotDotDotMenu: true
    },
  },
  // {
  //   path: "/networks-DEPRECATED",
  //   name: "Networks-DEPRECATED",
  //   // route level code-splitting
  //   // this generates a separate chunk (about.[hash].js) for this route
  //   // which is lazy-loaded when the route is visited.
  //   component: () =>
  //     import(
  //       /* webpackChunkName: "NetworkView-DEPRECATED" */ "./views/NetworkView-DEPRECATED.vue"
  //     ),
  //   meta: {
  //     breadcrumbs: [
  //       { text: "Home", href: "/#/" },
  //       { text: "Networks {deprecated}", disabled: true },
  //     ],
  //   },
  // },
  // {
  //   path: "/devices-DEPRECATED",
  //   name: "Devices {DEPRECATED}",
  //   // route level code-splitting
  //   // this generates a separate chunk (about.[hash].js) for this route
  //   // which is lazy-loaded when the route is visited.
  //   component: () =>
  //     import(/* webpackChunkName: "DeviceView-DEPRECATED" */ "./views/DeviceView-DEPRECATED.vue"),
  //   meta: {
  //     breadcrumbs: [
  //       { text: "Home", href: "/#/" },
  //       { text: "Devices{deprecated}", disabled: true },
  //     ],
  //   },
  // },

  // Always leave this as last one,
  // but you can also remove it
  {
    path: '/:catchAll(.*)*',
    component: () => import('pages/ErrorNotFound.vue'),
  },
];

export default routes;
