# WTF-UI (html-quasar)

WTF user interface

The html here gets built and embedded into the WTF application.

But for developent, it can be built and debugged like any other web application.

## Configure

Somewhere have access to a WTF backend running (either locally build and debug, or point at some other machine running).

Edit .env file as appropriate (perhaps based on .env.development or .env.production)

## Install the dependencies

```bash
npm install
```

### Start the app in development mode (hot-code reloading, error reporting, etc.)

```bash
npm run dev
```

### Lint the files

```bash
npm run lint
```

### Format the files

```bash
npm run format
```

### Build the app for production

```bash
quasar build
```

### Customize the configuration

See [Configuring quasar.config.js](https://v2.quasar.dev/quasar-cli-vite/quasar-config-js).
