// https://cli.vuejs.org/guide/mode-and-env.html
// https://alligator.io/vuejs/working-with-environment-variables/

export const API_ROOT = process.env.VUE_APP_ROOT_API ? process.env.VUE_APP_ROOT_API :
    window.location.protocol + '//' + window.location.hostname + ':' + process.env.VUE_APP_DEFAULT_API_PORT;

export const API_NETWORKS_PATH = process.env.VUE_APP_API_NETWORKS_PATH;

export const API_DEVICES_PATH = process.env.VUE_APP_API_DEVICES_PATH;

export const DEBUG_MODE = process.env.VUE_APP_DEBUG_MODE;
