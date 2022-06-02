// https://cli.vuejs.org/guide/mode-and-env.html
// https://alligator.io/vuejs/working-with-environment-variables/

export const API_ROOT: string = process.env.VUE_APP_ROOT_API ? process.env.VUE_APP_ROOT_API :
    window.location.protocol + "//" + window.location.hostname + ":" + process.env.VUE_APP_DEFAULT_API_PORT;

export const DEBUG_MODE: boolean = process.env.VUE_APP_DEBUG_MODE=='true';
