import store from "@/store/store";
import Vue from "vue";
import App from "./App.vue";
import vuetify from "./plugins/vuetify";
import VueMoment from "vue-moment";
import router from "./router";

Vue.config.productionTip = false;
Vue.use(VueMoment);

(window as any).myVueApp = new Vue({
  router,
  store,
  vuetify,
  render: (h) => h(App),
}).$mount("#app");
