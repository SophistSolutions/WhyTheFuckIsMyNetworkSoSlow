// https://quasar.dev/quasar-cli-webpack/handling-process-env

export const API_ROOT: string = process.env.VUE_APP_ROOT_API ? process.env.VUE_APP_ROOT_API :
    window.location.protocol + "//" + window.location.hostname + ":" + process.env.VUE_APP_DEFAULT_API_PORT;

export const DEBUG_MODE: boolean = process.env.VUE_APP_DEBUG_MODE=='true';

export const VUE_MY_HTML_APP_VERSION: string = process.env.VUE_MY_HTML_APP_VERSION ?? "";
export const VUE_VERSION: string = process.env.VUE_VERSION ?? "";

// console.log(`API_ROOT=` + API_ROOT)
// console.log(`DEBUG_MODE=` + DEBUG_MODE)
// console.log(`VUE_MY_HTML_APP_VERSION=` + VUE_MY_HTML_APP_VERSION)
// console.log(`VUE_VERSION=` + VUE_VERSION)
