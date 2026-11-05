# WTF's Docker image

`sophistsolutionsinc/whythefuckismynetworksoslow:<version>` - WTF installed from its Release `.deb` in Ubuntu 26.04,
for a Linux host. Using it is in [Documentation/Getting-Started.md](../../Documentation/Getting-Started.md).

- It must run with `--network host`: on Docker's default bridge network WTF would see only that network, not the LAN.
  So it is for Docker Engine on Linux, not Docker Desktop (which runs containers in a virtual machine).
- Its settings and database are in the volume at `/var/opt/WhyTheFuckIsMyNetworkSoSlow`. On each start the entrypoint
  writes `WTF_PORT`, if set, into its AppSettings.json as `WebServerPort`, and logs to stdout (`docker logs`) unless
  AppSettings.json says otherwise.

## Build and publish

In WTF-Dev-2604 (or any Linux with docker, and a Release build from Ubuntu 26.04):

```bash
make CONFIGURATION=Release all -j8
make -C Installers/Docker image      # sophistsolutionsinc/whythefuckismynetworksoslow:<VERSION>; DEB=... for another .deb
make -C Installers/Docker push       # docker login as sophists first; a final version is pushed as :latest too
```

## Two versions side by side

To compare this release with the next one's build on the same machine, run each with its own port and volume:

```bash
docker run -d --name wtf-1.0d21 --network host -e WTF_PORT=8081 \
    -v wtf-1.0d21:/var/opt/WhyTheFuckIsMyNetworkSoSlow sophistsolutionsinc/whythefuckismynetworksoslow:1.0d21
docker run -d --name wtf-next --network host -e WTF_PORT=8082 \
    -v wtf-next:/var/opt/WhyTheFuckIsMyNetworkSoSlow sophistsolutionsinc/whythefuckismynetworksoslow:1.0d22x
```

A new volume starts a new database; to try the next version on this one's data, copy the volume first
(`docker run --rm -v wtf-1.0d21:/from -v wtf-next:/to ubuntu cp -a /from/. /to/`).
