/* eslint-disable */
import moment from 'moment'

declare module '*.vue' {
  import type { DefineComponent } from 'vue'
  const component: DefineComponent<{}, {}, any>
  export default component
}
declare module 'vue-filter-pretty-bytes';

declare module '@vue/runtime-core' {
  export interface ComponentCustomProperties {
    $moment: typeof moment
    $validate: (data: object, rule: object) => boolean
  }
}
