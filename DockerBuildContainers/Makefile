export TOP_ROOT=$(abspath ../)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

DEFAULT_GOAL := help

VERSION=$(shell dos2unix < ../VERSION | xargs echo -n)
VERSION_Final=$(shell $(StroikaRoot)ScriptsLib/ExtractVersionInformation ../VERSION FinalBuild)
DETECTED_HOST_OS=$(shell $(StroikaRoot)ScriptsLib/DetectedHostOS)


help:
	@echo "make build-images to create the docker images on your computer"
	@echo "to run the image, use:"
	@echo "    docker run -it sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004   OR"
	@echo "n.b. use --security-opt seccomp=unconfined to run debugger (ptrace/address randomization disable)"
	@echo "and first thing when you run docker image, cat ReadMe.md"


# You may want to comment these in to build WTF-Dev environment
## @todo maybe cleaner to remove these build args and do WTF-DEV Dockerfile and have it import different bases and just add this stuff?
DOCKER_BUILD_EXTRAS_FOR_DEV_=\
	--build-arg INCLUDE_OPENSSL=true \
	--build-arg INCLUDE_HANDY_DEV_TOOLS=true \

DOCKER_BUILD_EXTRAS_?=\

build-images:
ifeq (${DETECTED_HOST_OS},Linux)
	docker build ${DOCKER_BUILD_EXTRAS_} -f Ubuntu-2004/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004 .
	docker build ${DOCKER_BUILD_EXTRAS_} -f Ubuntu-2204/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2204 .
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker build -f Windows-Cygwin-VS2k19/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19 -m 2G .
	docker build -f Windows-Cygwin-VS2k19/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k22 -m 2G .
endif


tag-images:
ifeq (${DETECTED_HOST_OS},Linux)
	docker tag sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004 sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004:$(VERSION)
	docker tag sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004 sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2204:$(VERSION)
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker tag sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19:latest sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19:$(VERSION)
	docker tag sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19:latest sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k22:$(VERSION)
endif


push-images:
	#if you get errors doing this (permission denied) make sure you ran docker login
ifeq (${DETECTED_HOST_OS},Linux)
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2204
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k22
endif
ifeq ($(VERSION_Final),true)
ifeq (${DETECTED_HOST_OS},Linux)
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004:$(VERSION)
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2204:$(VERSION)
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19:$(VERSION)
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k22:$(VERSION)
endif
endif


dev-containers:
ifeq (${DETECTED_HOST_OS},Linux)
	DOCKER_BUILD_EXTRAS_="$(DOCKER_BUILD_EXTRAS_FOR_DEV_)" make build-images
	cd ${StroikaRoot} && SANDBOX_FROM=/Sandbox CONTAINER_NAME=WTF-Dev-2004 CONTAINER_IMAGE=sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004 INCLUDE_EXTRA_PERSONAL_MOUNT_FILES=1 EXTRA_DOCKER_ARGS="--publish 11222:22" ECHO_DOCKER_COMMANDS=1 ScriptsLib/RunInDockerEnvironment
endif
