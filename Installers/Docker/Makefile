export TOP_ROOT=$(abspath ../../)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

.DEFAULT_GOAL := help

VERSION=$(shell tr -d '\r\n' < $(TOP_ROOT)VERSION)
VERSION_Final=$(shell $(StroikaRoot)Build/Scripts/ExtractVersionInformation $(TOP_ROOT)VERSION FinalBuild)

IMAGE?=sophistsolutionsinc/whythefuckismynetworksoslow
# The image is made from a Release .deb built on the base image's Ubuntu (26.04 - WTF-Dev-2604, or CI's ubuntu-26.04 job)
DEB?=$(TOP_ROOT)Builds/Release/WhyTheFuckIsMyNetworkSoSlow/whythefuckismynetworksoslow-Linux-$(VERSION)_x86_64.deb

help:
	@echo "make image [DEB=<a Release .deb>] - builds $(IMAGE):$(VERSION) from the .deb"
	@echo "make push - pushes it (docker login as sophists first), and as :latest too for a final (not 'x') version"
	@echo "ReadMe.md says how to run it"

image:
ifeq ($(wildcard $(DEB)),)
	$(error no .deb at $(DEB) - build Release first, or give DEB=...)
endif
	CTX=$$(mktemp -d) && cp Dockerfile entrypoint.sh $(DEB) $$CTX/ \
		&& docker build --build-arg DEB=$(notdir $(DEB)) -t $(IMAGE):$(VERSION) $$CTX; \
		RC=$$?; rm -rf $$CTX; exit $$RC

push:
	docker push $(IMAGE):$(VERSION)
ifeq ($(VERSION_Final),true)
	docker tag $(IMAGE):$(VERSION) $(IMAGE):latest
	docker push $(IMAGE):latest
endif
