DEFAULT_GOAL := help


help:
	@echo "make docker-images to create the docker images on your computer"
	@echo "to run the image, use:"
	@echo "    docker run -it sophistsolutionsinc/whythefuckismynetworksoslow-debian-small   OR"
	@echo "    docker run -it sophistsolutionsinc/whythefuckismynetworksoslow-debian-crosscompile-raspberrypi"
	@echo "n.b. use --security-opt seccomp=unconfined to run debugger (ptrace/address randomization disable)"
	@echo "and first thing when you run docker image, cat ReadMe.md"

docker-images:
ifeq ($(shell uname),Linux)
	docker build -f Debian-Small/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksoslow-debian-small .
	docker build -f Debian-CrossCompile-raspberrypi/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksoslow-debian-crosscompile-raspberrypi .
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker build -f Windows-Cygwin-VS2k19/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19 -m 2G .
	docker build -f Windows-MinGW-VS2k19/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksoslow-windows-mingw-vs2k19 -m 2G .
endif

push-images:
	#if you get errors doing this (permission denied) make sure you ran docker login
ifeq ($(shell uname),Linux)
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-debian-small
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-debian-crosscompile-raspberrypi
endif
ifeq (Windows,$(findstring Windows,$(OS)))
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19
	docker push sophistsolutionsinc/whythefuckismynetworksoslow-windows-mingw-vs2k19
endif

