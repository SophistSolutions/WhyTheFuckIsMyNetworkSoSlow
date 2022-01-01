import { createApp } from 'vue'
import router from './router'
import store from './store'
import vuetify from './plugins/vuetify'
import App from './App.vue'
import { loadFonts } from './plugins/webfontloader'
import JsonViewer from "vue-json-viewer"
import moment from 'moment';

loadFonts()

const app = createApp(App)
  .use(vuetify)
  .use(router)
  .use(JsonViewer)
  .use(store)

app.mount('#app')

app.config.globalProperties.$moment = moment
