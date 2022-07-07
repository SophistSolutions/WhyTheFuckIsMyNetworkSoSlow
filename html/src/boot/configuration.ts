import { boot } from 'quasar/wrappers';

import { kCompileTimeConfiguration } from '../config/config';
import { Logger } from '../utils/Logger';

export default boot(async (/* { app, router, ... } */) => {
  let API_ROOT: string | null = null;
  try {
    const r = await fetch('/config.json');
    if (r.ok) {
      const contentType: string | null = r.headers.get('content-type');
      if (contentType && contentType.indexOf('application/json') !== -1) {
        const cfgObj: any = r.json();
        if (
          cfgObj &&
          cfgObj['API_ROOT'] &&
          typeof cfgObj['API_ROOT'] === 'string'
        ) {
          API_ROOT = cfgObj['API_ROOT'];
        }
      }
    }
  } catch (e) {
    Logger.warn(e);
  }
  if (!API_ROOT) {
    API_ROOT = kCompileTimeConfiguration.APP_ROOT_API;
  }
  if (!API_ROOT) {
    API_ROOT =
      window.location.protocol +
      '//' +
      window.location.hostname +
      ':' +
      (kCompileTimeConfiguration.APP_DEFAULT_API_PORT ?? '8080');
  }
  gRuntimeConfiguration = {
    API_ROOT: API_ROOT ?? '',
  };
  if (kCompileTimeConfiguration.DEBUG_MODE) {
    console.log(
      `gRuntimeConfiguration.API_ROOT=` + gRuntimeConfiguration.API_ROOT
    );
  }
});

/*
 *  Configuration object known at runtime.
 *  The app should access its runtime specified configuration (mainly pointer to API-Server but could be other stuff communicated to app at startup, like
 *  debug flags etc?).
 */
export let gRuntimeConfiguration = {
  API_ROOT: '' as string,
};
