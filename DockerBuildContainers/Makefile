export TOP_ROOT=$(abspath ../)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

.DEFAULT_GOAL := help

VERSION=$(shell dos2unix < ../VERSION | xargs echo -n)
VERSION_Final=$(shell $(StroikaRoot)Build/Scripts/ExtractVersionInformation ../VERSION FinalBuild)
DETECTED_HOST_OS=$(shell $(StroikaRoot)Build/Scripts/DetectedHostOS)


help:
	@echo "make build-images to create the docker images on your computer"
	@echo "to run the image, use:"
	@echo "    docker run -it sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2604   OR"
	@echo "n.b. use --security-opt seccomp=unconfined to run debugger (ptrace/address randomization disable)"
	@echo "and first thing when you run docker image, cat ReadMe.md"
	@echo "make run-test-container WTF_TREE=... CONFIGURATION=... to run a WTF build on the host network (Linux)"
	@echo "make test-deb-install DEB=... to install a built .deb in a clean ubuntu:26.04 container and run it"


DOCKER_BUILD_EXTRAS_?=\

DEFAULT_IMAGES_?=

## NOTE: Since these can be slow/big, the IMAGES arg can be given on the commandline to override this
ifeq (${DETECTED_HOST_OS},Linux)
DEFAULT_IMAGES_+= sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2204
DEFAULT_IMAGES_+= sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2404
DEFAULT_IMAGES_+= sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2604
endif
ifneq ($(findstring $(DETECTED_HOST_OS),MSYS-Cygwin),)
DEFAULT_IMAGES_+= sophistsolutionsinc/whythefuckismynetworksoslow-windows-vs2k22
endif

IMAGES?=${DEFAULT_IMAGES_}

build-images:
	${MAKE} --no-print-directory ${IMAGES}


sophistsolutionsinc/whythefuckismynetworksoslow-windows-vs2k22:
	docker build -f Windows-VS2k22/Dockerfile -t $@ -m 2G .

sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2204:
	docker build -f Ubuntu-2204/Dockerfile -t $@ ${DOCKER_BUILD_EXTRAS_} .

sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2404:
	docker build -f Ubuntu-2404/Dockerfile -t $@ ${DOCKER_BUILD_EXTRAS_} .

sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2604:
	docker build -f Ubuntu-2604/Dockerfile -t $@ ${DOCKER_BUILD_EXTRAS_} .


tag-images:
	for image in ${IMAGES}; do \
		docker tag $${image} $${image}:v1; \
	done
# ifeq ($(STROIKA_VERSION_Final),true)
# 	for image in ${IMAGES}; do  \
# 		docker tag $${image} $${image}:${STROIKA_VERSION}; \
# 	done
# endif


push-images:
	#if you get errors doing this (permission denied) make sure you ran docker login (as sophists - else no permission to push these)
	for image in ${IMAGES}; do \
		docker push $${image}:latest; \
		docker push $${image}:v1; \
	done
# ifeq ($(STROIKA_VERSION_Final),true)
# 	for image in ${IMAGES}; do  \
# 		docker push $${image}:${STROIKA_VERSION}; \
# 	done
# endif


# WTF-Dev-NNNN: long-lived containers to work and debug in (ssh, VS Code Remote-SSH), made as Stroika's
# stroika-dev-containers makes its Stroika-Dev-NNNN: each is WTF's build image NNNN plus Stroika's dev layer
# (Build/Docker/Stroika-Dev/Dockerfile - sshd, dev tools, and the entrypoint that starts sshd and keeps the container's
# host keys), and mounts the host's /Sandbox. make dev-containers makes all three, make dev-container-2604 one; to
# recreate one, docker rm -f it first. (Run by hand, without a terminal: TERM=xterm CMD2RUN=true STOP_AT_END=0 make ...)
WTF_DEV_PORT_2204=11223
WTF_DEV_PORT_2404=11224
WTF_DEV_PORT_2604=11225
# Their sshd host keys, kept on the host so a recreated container keeps them - not under /Sandbox, which every container
# mounts (as Stroika's STROIKA_DEV_HOST_KEYS_DIR)
WTF_DEV_HOST_KEYS_DIR?=${HOME}/.local/state/wtf-dev-containers

dev-containers:
ifeq (${DETECTED_HOST_OS},Linux)
	for v in 2204 2404 2604; do $(MAKE) --no-print-directory dev-container-$$v || exit $$?; done
endif

# --init: Docker's init as PID 1 reaps what is orphaned into it (the entrypoint ends exec'ing sshd -D), as in Stroika's
dev-container-%: sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-%
	cd ${StroikaRoot}Build/Docker && docker build -f Stroika-Dev/Dockerfile --build-arg BASED_ON_IMAGE=sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-$* -t sophistsolutionsinc/whythefuckismynetworksoslow-dev-$* .
	mkdir -p ${WTF_DEV_HOST_KEYS_DIR}/WTF-Dev-$* && chmod 700 ${WTF_DEV_HOST_KEYS_DIR}/WTF-Dev-$*
	cd ${StroikaRoot} && SANDBOX_FROM=/Sandbox CONTAINER_NAME=WTF-Dev-$* CONTAINER_IMAGE=sophistsolutionsinc/whythefuckismynetworksoslow-dev-$* INCLUDE_EXTRA_PERSONAL_MOUNT_FILES=1 EXTRA_DOCKER_ARGS="--init --publish ${WTF_DEV_PORT_$*}:22 --restart=unless-stopped -v ${WTF_DEV_HOST_KEYS_DIR}/WTF-Dev-$*:/etc/ssh/persistent-host-keys" ECHO_DOCKER_COMMANDS=1 ACTUAL_RUN_CMD="/usr/local/bin/stroika-dev-entrypoint.sh" Build/Scripts/RunInDockerEnvironment

start-dev-containers:
ifeq (${DETECTED_HOST_OS},Linux)
	for i in WTF-Dev-2204 WTF-Dev-2404 WTF-Dev-2604; do docker start $$i; done
endif


# Run a WTF build from a tree on this host, on the host network - so discovery sees the real LAN - side by side with an
# installed WTF service: its own data volume (AppSettings.json and the database), seeded with a WebServerPort other
# than the service's 80. Ctrl+C stops it. The tree is mounted at its own path, so its links still resolve.
#	make run-test-container WTF_TREE=/Sandbox/claude/WTF-protagoras-2604 CONFIGURATION=Debug
WTF_TREE?=$(abspath ..)
CONFIGURATION?=Debug
TEST_CONTAINER_IMAGE?=sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2604
TEST_CONTAINER_DATA_VOLUME?=wtf-test-data
TEST_CONTAINER_PORT?=8080

run-test-container:
ifneq (${DETECTED_HOST_OS},Linux)
	$(error run-test-container needs a Linux host - docker's host network)
endif
	docker run --rm -it --network host \
		-v ${WTF_TREE}:${WTF_TREE} \
		-v ${TEST_CONTAINER_DATA_VOLUME}:/var/opt/WhyTheFuckIsMyNetworkSoSlow \
		${TEST_CONTAINER_IMAGE} \
		bash -c 'test -f /var/opt/WhyTheFuckIsMyNetworkSoSlow/AppSettings.json || echo "{ \"WebServerPort\": ${TEST_CONTAINER_PORT} }" > /var/opt/WhyTheFuckIsMyNetworkSoSlow/AppSettings.json; exec ${WTF_TREE}/Builds/${CONFIGURATION}/WhyTheFuckIsMyNetworkSoSlow/WhyTheFuckIsMyNetworkSoSlow-Backend --run-directly'


# Install a built .deb in a clean Ubuntu container - it has no systemd, so the postinst skips systemctl - and run the
# installed program: catches a missing package dependency (the control file's Depends: is empty).
#	make test-deb-install DEB=/Sandbox/claude/WTF-protagoras-2604/Builds/Release/WhyTheFuckIsMyNetworkSoSlow/<the .deb>
TEST_DEB_IMAGE?=ubuntu:26.04

test-deb-install:
ifeq (${DEB},)
	$(error test-deb-install needs DEB=<path to a .deb>)
endif
	docker run --rm -v $(abspath $(dir ${DEB})):/debs:ro ${TEST_DEB_IMAGE} \
		bash -c 'apt-get -qq update > /dev/null && apt-get -qq install -y /debs/$(notdir ${DEB}) && /opt/WhyTheFuckIsMyNetworkSoSlow/WhyTheFuckIsMyNetworkSoSlow-Backend --version'
