DEFAULT_GOAL := help


help:
	@echo "make docker-images to create the docker images on your computer"
	@echo "to run the image, use:"
	@echo "    docker run -it sophistsolutionsinc/whythefuckismynetworksosmall-debian-small   OR"
	@echo "    docker run -it sophistsolutionsinc/whythefuckismynetworksosmall-debian-crosscompile-raspberrypi"
	@echo "n.b. use --security-opt seccomp=unconfined to run debugger (ptrace/address randomization disable)"
	@echo "and first thing when you run docker image, cat ReadMe.md"

docker-images:
	docker build -f Debian-Small/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksosmall-debian-small .
	docker build -f Debian-CrossCompile-raspberrypi/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksosmall-debian-crosscompile-raspberrypi .

push-images:
	#if you get errors doing this (permission denied) make sure you ran docker login
	docker push sophistsolutionsinc/whythefuckismynetworksosmall-debian-small
	docker push sophistsolutionsinc/whythefuckismynetworksosmall-debian-crosscompile-raspberrypi
