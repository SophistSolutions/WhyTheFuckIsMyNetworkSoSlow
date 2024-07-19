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



## NOTE: Since these can be slow/big, the IMAGES arg can be given on the commandline to override this
ifeq (${DETECTED_HOST_OS},Linux)
IMAGES+= sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2204
IMAGES+= sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2404
endif
ifeq (Windows,$(findstring Windows,$(DETECTED_HOST_OS)))
IMAGES+= sophistsolutionsinc/whythefuckismynetworksoslow-windows-vs2k22
endif


build-images:
	${MAKE} --no-print-directory ${IMAGES}


sophistsolutionsinc/whythefuckismynetworksoslow-windows-vs2k22:
	docker build -f Windows-VS2k22/Dockerfile -t $@ -m 2G .

sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2204:
	docker build -f Ubuntu-2204/Dockerfile -t $@ .

sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2404:
	docker build -f Ubuntu-2404/Dockerfile -t $@ .


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


dev-containers:
ifeq (${DETECTED_HOST_OS},Linux)
	DOCKER_BUILD_EXTRAS_="$(DOCKER_BUILD_EXTRAS_FOR_DEV_)" make build-images
	cd ${StroikaRoot} && SANDBOX_FROM=/Sandbox CONTAINER_NAME=WTF-Dev-2004 CONTAINER_IMAGE=sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004 INCLUDE_EXTRA_PERSONAL_MOUNT_FILES=1 EXTRA_DOCKER_ARGS="--publish 11222:22" ECHO_DOCKER_COMMANDS=1 ScriptsLib/RunInDockerEnvironment
endif
