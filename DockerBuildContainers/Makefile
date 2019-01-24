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
