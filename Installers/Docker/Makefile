export TOP_ROOT=$(abspath ../../)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

.DEFAULT_GOAL := help

VERSION=$(shell tr -d '\r\n' < $(TOP_ROOT)VERSION)
VERSION_Final=$(shell $(StroikaRoot)Build/Scripts/ExtractVersionInformation $(TOP_ROOT)VERSION FinalBuild)

IMAGE?=sophistsolutionsinc/whythefuckismynetworksoslow
# The image is made from a Release .deb built on the base image's Ubuntu (26.04 - WTF-Dev-2604, or CI's ubuntu-26.04 job)
DEB?=$(TOP_ROOT)Builds/Release/WhyTheFuckIsMyNetworkSoSlow/whythefuckismynetworksoslow-Linux-$(VERSION)_x86_64.deb

# The port check runs the image on
CHECK_PORT?=18099

help:
	@echo "make image [DEB=<a Release .deb>] - builds $(IMAGE):$(VERSION) from the .deb"
	@echo "make check - runs it on port $(CHECK_PORT): the API must answer, and docker stop must stop it cleanly"
	@echo "make push - pushes it (docker login as sophists first), and as :latest too for a final (not 'x') version"
	@echo "make push-dev - pushes it as :dev, the newest development build"
	@echo "ReadMe.md says how to run it; CI (build-N-test.yml) does image, check and push or push-dev"

image:
ifeq ($(wildcard $(DEB)),)
	$(error no .deb at $(DEB) - build Release first, or give DEB=...)
endif
	CTX=$$(mktemp -d) && cp Dockerfile entrypoint.sh $(DEB) $$CTX/ \
		&& docker build --build-arg DEB=$(notdir $(DEB)) -t $(IMAGE):$(VERSION) $$CTX; \
		RC=$$?; rm -rf $$CTX; exit $$RC

check:
	docker run -d --name wtf-check --network host -e WTF_PORT=$(CHECK_PORT) $(IMAGE):$(VERSION) >/dev/null
	for i in $$(seq 1 60); do curl -sf -m 3 -o /dev/null http://localhost:$(CHECK_PORT)/api/v1/about && break; sleep 1; done; \
		curl -sf -m 5 http://localhost:$(CHECK_PORT)/api/v1/about; RC=$$?; echo; \
		docker stop wtf-check >/dev/null && docker inspect -f 'docker stop: exit code {{.State.ExitCode}}' wtf-check; \
		test "$$(docker inspect -f '{{.State.ExitCode}}' wtf-check)" = 0 || RC=1; \
		docker logs wtf-check 2>&1 | tail -5; docker rm -v wtf-check >/dev/null; exit $$RC

push:
	docker push $(IMAGE):$(VERSION)
ifeq ($(VERSION_Final),true)
	docker tag $(IMAGE):$(VERSION) $(IMAGE):latest
	docker push $(IMAGE):latest
endif

push-dev:
	docker tag $(IMAGE):$(VERSION) $(IMAGE):dev
	docker push $(IMAGE):dev
