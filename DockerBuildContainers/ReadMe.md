# Docker Build Containers

There is no need to use this, but it can provide a quickstart to building WhyTheFuckIsMyNetworkSoSlow

Just create one or more of these docker images, and run them as in:

- `make docker-images`
- `docker run -it sophistsolutionsinc/whythefuckismynetworksosmall-debian-small`
  - then inside the docker container find a ReadMe.md, and follow its instructions to build (basically clone WhyTheFuckIsMyNetworkSoSlow and type make )

## Create and setup for ssh WTF-Dev images

(used for LGP development - very optional)

- `make dev-containers`
- `for i in Stroika-Dev Stroika-Dev-1804 Stroika-Dev-2004 Stroika-Dev-2110 Stroika-Dev-2204; do docker start $i; docker exec -it $i sudo service ssh start; done`
