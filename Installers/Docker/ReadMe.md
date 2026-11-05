# WTF's Docker image

`sophistsolutionsinc/whythefuckismynetworksoslow:<version>` - WTF installed from its Release `.deb` in Ubuntu 26.04,
for a Linux host. Using it is in [Documentation/Getting-Started.md](../../Documentation/Getting-Started.md).

- It must run with `--network host`: on Docker's default bridge network WTF would see only that network, not the LAN.
  So it is for Docker Engine on Linux, not Docker Desktop (which runs containers in a virtual machine).
- Its settings and database are in the volume at `/var/opt/WhyTheFuckIsMyNetworkSoSlow`. On each start the entrypoint
  writes `WTF_PORT`, if set, into its AppSettings.json as `WebServerPort`, and logs to stdout (`docker logs`) unless
  AppSettings.json says otherwise.

## Tags

- `:dev` - the newest development build: CI pushes it on every push to `v1-Dev`.
- `:<version>` (`:1.0d21`, say) - a release: CI pushes it on a push to `v1-Release`; and `:latest`, the newest release.

## Build and publish

CI does this (`build-N-test.yml`, its `docker-image` job), from the ubuntu-26.04 Release job's .deb. By hand - to try a
change before pushing it - in WTF-Dev-2604 (or any Linux with docker, and a Release build from Ubuntu 26.04):

```bash
make CONFIGURATION=Release all -j8
make -C Installers/Docker image      # sophistsolutionsinc/whythefuckismynetworksoslow:<VERSION>; DEB=... for another .deb
make -C Installers/Docker check      # runs it: the API must answer, and docker stop must stop it cleanly
make -C Installers/Docker push       # docker login as sophists first; a final version is pushed as :latest too
```

## Two versions side by side

To compare this release with the next one's build on the same machine, run each with its own port and volume:

```bash
docker run -d --name wtf-1.0d21 --network host -e WTF_PORT=8081 \
    -v wtf-1.0d21:/var/opt/WhyTheFuckIsMyNetworkSoSlow sophistsolutionsinc/whythefuckismynetworksoslow:1.0d21
docker run -d --name wtf-next --network host -e WTF_PORT=8082 \
    -v wtf-next:/var/opt/WhyTheFuckIsMyNetworkSoSlow sophistsolutionsinc/whythefuckismynetworksoslow:dev
```

A new volume starts a new database; to try the next version on this one's data, copy the volume first
(`docker run --rm -v wtf-1.0d21:/from -v wtf-next:/to ubuntu cp -a /from/. /to/`).
