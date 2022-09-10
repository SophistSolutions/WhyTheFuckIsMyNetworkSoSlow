# WhyTheFuckIsMyNetworkSoSlow

This project's goal is to allow people (even with modest knowledge of network technology) to
diagnose their own personal networks, to indicate network issues and to provide advice for improving network efficiency.

> **_Note:_** - as of the current release, all you can really do effectively is to explore the devices present on your network (including history) (not doing much monitoring yet, except manually through the web API).

- To test:

  - Install (msi/deb/rpm installs in release section below)

    installation starts background web-service, on port 80 for GUI, and 8080 for API-Server

  - In a browser, visit:
    - <http://localhost/> (web-gui)
    - <http://localhost/api> (web-api)

- TODO items
  See <https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues>

## Releases (and built installers)

- [Github WhyTheFuckIsMyNetworkSoSlow Releases Page](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/releases)

## CI System Integration

- Github Actions

  | [Branches](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow) |                                                                                                                                              Status                                                                                                                                               |                                                                                                |
  | :-------------------------------------------------------------------------- | :-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------: | :--------------------------------------------------------------------------------------------- |
  | **v1-Release**                                                              | [![build-N-test-v1-Release](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/workflows/build-N-test-v1-Release/badge.svg?branch=v1-Release)](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/actions?query=workflow%3Abuild-N-test-v1-Release+branch%3Av1-Release) | [.github/workflows/build-N-test-v1-Release.yml](.github/workflows/build-N-test-v1-Release.yml) |
  | **v1-Dev**                                                                  |           [![build-N-test-v1-Dev](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/workflows/build-N-test-v1-Dev/badge.svg?branch=v1-Dev)](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/actions?query=workflow%3Abuild-N-test-v1-Dev+branch%3Av1-Dev)           | [.github/workflows/build-N-test-v1-Dev.yml](.github/workflows/build-N-test-v1-Dev.yml)         |
