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
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker build -f Windows-Cygwin-VS2k19/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19 -m 2G .
endif


tag-images:
ifeq ($(shell uname),Linux)
	docker tag sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804 sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804:$(VERSION)
	docker tag sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004 sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004:$(VERSION)
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker tag sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19:latest sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19:$(VERSION)
endif


push-images:
	#if you get errors doing this (permission denied) make sure you ran docker login
ifeq ($(shell uname),Linux)
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19
endif
ifeq ($(VERSION_Final),true)
ifeq ($(shell uname),Linux)
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804:$(VERSION)
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004:$(VERSION)
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19:$(VERSION)
endif
endif
