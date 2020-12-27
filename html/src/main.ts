import store from "@/store/store";
import Vue from "vue";
import JsonViewer from "vue-json-viewer";
import VueMoment from "vue-moment";
import App from "./App.vue";
import vuetify from "./plugins/vuetify";
import router from "./router";

Vue.config.productionTip = false;
Vue.use(VueMoment);
Vue.use(JsonViewer);

(window as any).myVueApp = new Vue({
  router,
  store,
  vuetify,
  render: (h) => h(App),
}).$mount("#app");
