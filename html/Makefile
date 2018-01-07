TOP_ROOT=$(shell realpath ../)/
StroikaPlatformTargetBuildDir=$(TOP_ROOT)Builds/$(CONFIGURATION)/
StroikaRoot=$(realpath $(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot)/

SHELL=/bin/bash
include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk



all:
	gulp


clean:
	rm -f src/*.js	

clobber: clean
	rm -rf dist

update-components:
	# automate/autodetect - @todo
	npm install
	#figure out why this is needed and remove
	npm install gulp-cli -g



check-prerequisites:
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Checking html prerequisites:"
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader.sh $$(($(MAKE_INDENT_LEVEL)+1)) && sh -c "(type node 2> /dev/null) || ($(StroikaRoot)/ScriptsLib/GetMessageForMissingTool.sh node && exit 1)"
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $$(($(MAKE_INDENT_LEVEL)+1)) && sh -c "(type npm 2> /dev/null) || ($(StroikaRoot)ScriptsLib/GetMessageForMissingTool.sh npm && exit 1)"
