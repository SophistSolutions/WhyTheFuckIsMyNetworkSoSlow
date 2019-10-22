export TOP_ROOT=$(abspath ../)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

include $(StroikaRoot)ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk


all:
ifeq ($(CONFIGURATION),)
	@echo "Cannot complete build without a configuraiton" && exit 1;
endif
	@$(MAKE) update-components
	@${MAKE} build-dist
	@rm -rf ${TOP_ROOT}/Builds/${CONFIGURATION}/WhyTheFuckIsMyNetworkSoSlow/html && mkdir -p ${TOP_ROOT}/Builds/${CONFIGURATION}/WhyTheFuckIsMyNetworkSoSlow/html && cp -r dist/* ${TOP_ROOT}/Builds/${CONFIGURATION}/WhyTheFuckIsMyNetworkSoSlow/html/

clean:
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Clean HTML:"
	@rm -f src/*.js

clobber:
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Clobber HTML:"
	@rm -f src/*.js
	@rm -rf dist node_modules

update-components:
	# automate/autodetect - @todo
	# @todo move build to IntermediateFiles, but I dont know npm well enuf yet... - that will get rid of need for ci
	npm ci

build-dist:
	npm run build

check-prerequisites:
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Checking html prerequisites:"
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $$(($(MAKE_INDENT_LEVEL)+1)) && sh -c "(type node 2> /dev/null) || ($(StroikaRoot)/ScriptsLib/GetMessageForMissingTool node && exit 1)"
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $$(($(MAKE_INDENT_LEVEL)+1)) && sh -c "(type npm 2> /dev/null) || ($(StroikaRoot)ScriptsLib/GetMessageForMissingTool npm && exit 1)"
