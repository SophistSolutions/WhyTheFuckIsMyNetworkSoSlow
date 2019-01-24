DEFAULT_GOAL := help


help:
	echo "make docker-images to create the docker images on your computer"
	echo "make run-image create the docker image in detatched mode (so you must start it and exec -it it yourself); not really useful but mostly example"

docker-images:
	docker build -f Debian-Small/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksosmall-debian-small .
	docker build -f Debian-CrossCompile-raspberrypi/Dockerfile -t sophistsolutionsinc/whythefuckismynetworksosmall-debian-crosscompile-raspberrypi .

push-images:
	#if you get errors doing this (permission denied) make sure you ran docker login
	docker push sophistsolutionsinc/whythefuckismynetworksosmall-debian-small
	docker push sophistsolutionsinc/whythefuckismynetworksosmall-debian-crosscompile-raspberrypi

run-image:
	IMAGE=${1:-sophistsolutionsinc/stroika-buildvm-ubuntu1804-small}; \
	docker run --security-opt seccomp=unconfined -d $$IMAGE
