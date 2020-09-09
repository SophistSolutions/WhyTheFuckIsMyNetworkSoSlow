DEFAULT_GOAL := help


help:
	@echo "make build-images to create the docker images on your computer"
	@echo "to run the image, use:"
	@echo "    docker run -it sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804   OR"
	@echo "n.b. use --security-opt seccomp=unconfined to run debugger (ptrace/address randomization disable)"
	@echo "and first thing when you run docker image, cat ReadMe.md"

build-images:
ifeq ($(shell uname),Linux)
	docker build -f Ubuntu-1804/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804 .
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker build -f Windows-Cygwin-VS2k19/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19 -m 2G .
endif

push-images:
	#if you get errors doing this (permission denied) make sure you ran docker login
ifeq ($(shell uname),Linux)
	docker push sophistsolutionsinc/sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19
endif

