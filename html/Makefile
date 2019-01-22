TOP_ROOT=$(shell realpath ../)/
export StroikaRoot?=$(realpath $(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot)/

include $(StroikaRoot)/ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk



all:
	#tmphack - workaround fact that gulp stuff broken for now
	mkdir -p dist
	-gulp


clean:
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Clean HTML:"
	@rm -f src/*.js	

clobber: 
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Clobber HTML:"
	@rm -f src/*.js	
	@rm -rf dist


update-components:
	# automate/autodetect - @todo
	npm install
	#figure out why this is needed and remove
	npm install gulp-cli -g


check-prerequisites:
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Checking html prerequisites:"
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $$(($(MAKE_INDENT_LEVEL)+1)) && sh -c "(type node 2> /dev/null) || ($(StroikaRoot)/ScriptsLib/GetMessageForMissingTool node && exit 1)"
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $$(($(MAKE_INDENT_LEVEL)+1)) && sh -c "(type npm 2> /dev/null) || ($(StroikaRoot)ScriptsLib/GetMessageForMissingTool npm && exit 1)"
