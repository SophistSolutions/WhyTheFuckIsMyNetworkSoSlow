# Docker Build Containers

There is no need to use this, but it can provide a quickstart to building WhyTheFuckIsMyNetworkSoSlow

Just create one or more of these docker images, and run them as in:

- `make build-images` (or one: `make IMAGES=sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2604 build-images`)
- `docker run -it sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2604`
  - then inside the docker container find Getting-Started.md, and follow its instructions to build (basically clone
    WhyTheFuckIsMyNetworkSoSlow and type make)

The images are Ubuntu 22.04, 24.04 and 26.04 (and Windows/VS2k22 and VS2k26), each on the matching Stroika build image,
plus a g++, an ARM (raspberrypi) cross compiler, and Node for the html. CI builds in them (build-N-test-Matrix.json), and
the build-Dev-Docker-Containers workflow (run by hand) publishes them.

## Run a build on the host network

`make run-test-container WTF_TREE=/Sandbox/... CONFIGURATION=Debug` (Linux) runs that tree's build on the host network,
so discovery sees the real LAN - side by side with an installed WTF service: it keeps its AppSettings.json and database
in its own docker volume (`TEST_CONTAINER_DATA_VOLUME`, default wtf-test-data), and listens on `TEST_CONTAINER_PORT`
(default 18080), not the service's (80 by default; 8080 on medusa). `TEST_CONTAINER_DOCKER_ARGS` (default `-it`) can run
it in the background instead, and cap it: `"-d --name wtf-test --cpus=2"`.

`make test-deb-install DEB=<path to a .deb>` installs a built .deb in a clean ubuntu:26.04 container, and runs it - to
catch a missing package dependency.

## WTF-Dev containers, to work in over ssh

(used for LGP development - very optional)

Long-lived containers on a Linux host, made as Stroika's Stroika-Dev ones are: WTF's build image plus Stroika's dev layer
(sshd, dev tools), mounting the host's /Sandbox. Connect with ssh or VS Code Remote-SSH, as the host's user.

- `make dev-containers` (or one: `make dev-container-2604`) - WTF-Dev-2204, WTF-Dev-2404 and WTF-Dev-2604, ssh on ports
  11223, 11224 and 11225. They restart with the host; their sshd host keys are kept in
  `~/.local/state/wtf-dev-containers/`, so recreating one (`docker rm -f` it, then make it again) keeps its keys.
- `make start-dev-containers` - starts them again after a `docker stop`.
