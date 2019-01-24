DEFAULT_GOAL := help


help:
	echo "make create-images to create the docker images on your computer"
	echo "make run-image create the docker image in detatched mode (so you must start it and exec -it it yourself); not really useful but mostly example"

create-images:
	docker build Debian-Small/ -t sophistsolutionsinc/whythefuckismynetworksosmall-debian-small
	docker build Debian-CrossCompile-raspberrypi/ -t sophistsolutionsinc/whythefuckismynetworksosmall-debian-crosscompile-raspberrypi

push-images:
	#if you get errors doing this (permission denied) make sure you ran docker login
	docker push sophistsolutionsinc/whythefuckismynetworksosmall-debian-small
	docker push sophistsolutionsinc/whythefuckismynetworksosmall-debian-crosscompile-raspberrypi

run-image:
	IMAGE=${1:-sophistsolutionsinc/stroika-buildvm-ubuntu1804-small}; \
	docker run --security-opt seccomp=unconfined -d $$IMAGE
