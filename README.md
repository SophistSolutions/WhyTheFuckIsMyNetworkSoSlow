# WhyTheFuckIsMyNetworkSoSlow

This project's goal is to allow people (even with modest knowledge of network technology) to
diagnose their own personal networks, to indicate network issues and to provide advice for improving network efficiency.

> **_Note:_** - as of the current release, all you can really do effectively is to explore the devices present on your network (including history) (not doing much monitoring yet, except manually through the web API).

## Try it

- **[Getting started](Documentation/Getting-Started.md)** - install it (Windows, Ubuntu, or Docker on Linux), what to
  expect in the first minutes, settings, logs, uninstalling.
- Installers: the [Releases page](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/releases). The Windows
  installer is not signed: if Windows says "Windows protected your PC", click More info, then Run anyway.
- Once installed, WTF runs as a background service, serving its web page and its API on one port - 80 by default
  (WebServerPort in AppSettings.json). In a browser, visit:
  - <http://localhost/> (web-gui)
  - <http://localhost/api> (web-api)

## Issues and plans

- <https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues>

## CI System Integration

- Github Actions - [.github/workflows/build-N-test.yml](.github/workflows/build-N-test.yml)

  | [Branches](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow) | Status |
  | :-------------------------------------------------------------------------- | :----: |
  | **v1-Release** | [![build-N-test](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/actions/workflows/build-N-test.yml/badge.svg?branch=v1-Release)](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/actions/workflows/build-N-test.yml?query=branch%3Av1-Release) |
  | **v1-Dev** | [![build-N-test](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/actions/workflows/build-N-test.yml/badge.svg?branch=v1-Dev)](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/actions/workflows/build-N-test.yml?query=branch%3Av1-Dev) |
