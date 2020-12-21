# WhyTheFuckIsMyNetworkSoSlow

This project's goal is to allow people (even with modest knowledge of network technology) to
diagnose their own personal networks, to indicate network issues and to provide advice for improving network efficiency.

- To test:

  - curl http://localhost/ OR
  - curl http://localhost:8080/ OR

- TODO items
  See <https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues>

## Releases (and built installers)

- [Github WhyTheFuckIsMyNetworkSoSlow Releases Page](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/releases)

## CI System Integration

- CircleCI

  - Only Release builds due to cost issues

  | [Branches](https://app.circleci.com/pipelines/github/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow) |                                                                                                 Status                                                                                                 | [.circleci/config.yml](.circleci/config.yml) |
  | :------------------------------------------------------------------------------------------------- | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------: | :------------------------------------------- |
  | **v1-Release**                                                                                     | [![Link](https://circleci.com/gh/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/tree/v1-Release.svg?style=shield)](https://circleci.com/gh/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/tree/v1-Release) |                                              |

- Github Actions

  | [Branches](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow) |                                                                                                                                                          Status                                                                                                                                                           |                                                                                                                |
  | :-------------------------------------------------------------------------- | :-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------: | :------------------------------------------------------------------------------------------------------------- |
  | **v1-Release**                                                              |    [![build-N-test-Linux-v1-Release](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/workflows/build-N-test-Linux-v1-Release/badge.svg?branch=v1-Release)](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/actions?query=workflow%3Abuild-N-test-Linux-v1-Release+branch%3Av1-Release)    | [.github/workflows/build-N-test-Linux-v1-Release.yml](.github/workflows/build-N-test-Linux-v1-Release.yml)     |
  |                                                                             |    [![build-N-test-MacOS-v1-Release](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/workflows/build-N-test-MacOS-v1-Release/badge.svg?branch=v1-Release)](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/actions?query=workflow%3Abuild-N-test-MacOS-v1-Release+branch%3Av1-Release)    | [.github/workflows/build-N-test-MacOS-v1-Release.yml](.github/workflows/build-N-test-MacOS-v1-Release.yml)     |
  |                                                                             | [![build-N-test-Windows-v1-Release](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/workflows/build-N-test-Windows-v1-Release/badge.svg?branch=v1-Release)](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/actions?query=workflow%3Abuild-N-test-Windows-v1-Release+branch%3Av1-Release) | [.github/workflows/build-N-test-Windows-v1-Release.yml](.github/workflows/build-N-test-Windows-v1-Release.yml) |
  | **v1-Dev**                                                                  |              [![build-N-test-Linux-v1-Dev](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/workflows/build-N-test-Linux-v1-Dev/badge.svg?branch=v1-Dev)](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/actions?query=workflow%3Abuild-N-test-Linux-v1-Dev+branch%3Av1-Dev)              | [.github/workflows/build-N-test-Linux-v1-Dev.yml](.github/workflows/build-N-test-Linux-v1-Dev.yml)             |
  |                                                                             |              [![build-N-test-MacOS-v1-Dev](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/workflows/build-N-test-MacOS-v1-Dev/badge.svg?branch=v1-Dev)](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/actions?query=workflow%3Abuild-N-test-MacOS-v1-Dev+branch%3Av1-Dev)              | [.github/workflows/build-N-test-MacOS-v1-Dev.yml](.github/workflows/build-N-test-MacOS-v1-Dev.yml)             |
  |                                                                             |           [![build-N-test-Windows-v1-Dev](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/workflows/build-N-test-Windows-v1-Dev/badge.svg?branch=v1-Dev)](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/actions?query=workflow%3Abuild-N-test-Windows-v1-Dev+branch%3Av1-Dev)           | [.github/workflows/build-N-test-Windows-v1-Dev.yml](.github/workflows/build-N-test-Windows-v1-Dev.yml)         |
