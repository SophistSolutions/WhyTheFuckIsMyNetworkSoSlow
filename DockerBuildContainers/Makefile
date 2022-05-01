export TOP_ROOT=$(abspath ../)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

DEFAULT_GOAL := help

VERSION=$(shell dos2unix < ../VERSION | xargs echo -n)
VERSION_Final=$(shell $(StroikaRoot)ScriptsLib/ExtractVersionInformation ../VERSION FinalBuild)


help:
	@echo "make build-images to create the docker images on your computer"
	@echo "to run the image, use:"
	@echo "    docker run -it sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804   OR"
	@echo "n.b. use --security-opt seccomp=unconfined to run debugger (ptrace/address randomization disable)"
	@echo "and first thing when you run docker image, cat ReadMe.md"


build-images:
ifeq ($(shell uname),Linux)
	docker build -f Ubuntu-1804/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804 .
	docker build -f Ubuntu-2004/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004 .
	docker build -f Ubuntu-2204/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2204 .
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker build -f Windows-Cygwin-VS2k19/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19 -m 2G .
endif


tag-images:
ifeq ($(shell uname),Linux)
	docker tag sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804 sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804:$(VERSION)
	docker tag sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004 sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004:$(VERSION)
	docker tag sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004 sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2204:$(VERSION)
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker tag sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19:latest sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19:$(VERSION)
endif


push-images:
	#if you get errors doing this (permission denied) make sure you ran docker login
ifeq ($(shell uname),Linux)
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2204
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19
endif
ifeq ($(VERSION_Final),true)
ifeq ($(shell uname),Linux)
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804:$(VERSION)
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004:$(VERSION)
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2204:$(VERSION)
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19:$(VERSION)
endif
endif


dev-containers:
ifeq ($(shell uname),Linux)
	#cd .. && SANDBOX_FROM=/Sandbox CONTAINER_NAME=Stroika-Dev CONTAINER_IMAGE=sophistsolutionsinc/stroika-dev INCLUDE_EXTRA_PERSONAL_MOUNT_FILES=1 EXTRA_DOCKER_ARGS="--publish 10022:22" ECHO_DOCKER_COMMANDS=1 ScriptsLib/RunInDockerEnvironment
	cd ../ThirdPartyComponents/Stroika/StroikaRoot/ && SANDBOX_FROM=/Sandbox CONTAINER_NAME=WTF-Dev-1804 CONTAINER_IMAGE=sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804 INCLUDE_EXTRA_PERSONAL_MOUNT_FILES=1 ECHO_DOCKER_COMMANDS=1 ScriptsLib/RunInDockerEnvironment
	#cd .. && SANDBOX_FROM=/Sandbox CONTAINER_NAME=Stroika-Dev-2004 CONTAINER_IMAGE=sophistsolutionsinc/stroika-dev-2004 INCLUDE_EXTRA_PERSONAL_MOUNT_FILES=1 EXTRA_DOCKER_ARGS="--publish 10222:22" ECHO_DOCKER_COMMANDS=1 ScriptsLib/RunInDockerEnvironment
	#cd .. && SANDBOX_FROM=/Sandbox CONTAINER_NAME=Stroika-Dev-2110 CONTAINER_IMAGE=sophistsolutionsinc/stroika-dev-2110 INCLUDE_EXTRA_PERSONAL_MOUNT_FILES=1 EXTRA_DOCKER_ARGS="--publish 10322:22" ECHO_DOCKER_COMMANDS=1 ScriptsLib/RunInDockerEnvironment
endif
