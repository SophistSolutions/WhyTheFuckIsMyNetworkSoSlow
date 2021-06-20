import store from "@/store/store";
import App from "./App.vue";
import vuetify from "./plugins/vuetify";
import router from "./router";
import Vue from "vue";
import vueFilterPrettyBytes from "vue-filter-pretty-bytes";
import JsonViewer from "vue-json-viewer";
import VueMoment from "vue-moment";

Vue.config.productionTip = false;
Vue.use(VueMoment);
Vue.use(JsonViewer);
Vue.use(vueFilterPrettyBytes);

(window as any).myVueApp = new Vue({
  router,
  store,
  vuetify,
  render: (h) => h(App),
}).$mount("#app");
