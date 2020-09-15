export TOP_ROOT?=$(shell realpath ../../)/
export StroikaRoot?=$(shell realpath ../Stroika/StroikaRoot)/

include $(StroikaRoot)ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk

BUILDS_DIR=../../Builds/$(CONFIGURATION)/ThirdPartyComponents/share/
OUT_FILE=$(BUILDS_DIR)OSI-MAC-PREFIXES.txt

### NOTES:
### This data is originally from http://standards.ieee.org/regauth/oui/oui.txt - I think, but
### that no longer exists.
### This seems to work adequately for now.
###
### @todo Find mirorrs and perhaps post-process
### @todo figure out if there are any licensing issues with using this data? Hopefully the fact
### that I dont distribute it, but reference it, side-steps such issues (but unclear).
### \note - reliably fails to download from TRAVIS-CI machines, so mirror nmap-mac-prefixes
FETCHURLS=
FETCHURLS+=https://raw.githubusercontent.com/nmap/nmap/master/nmap-mac-prefixes
FETCHURLS+=ftp://ftp.sophists.com/Pub/Mirror/nmap-mac-prefixes


all:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "ThirdPartyComponents/OSI-MAC-Prefixes Build {$(CONFIGURATION)}:"
	@$(StroikaRoot)ScriptsLib/CheckValidConfiguration $(CONFIGURATION)
	@$(MAKE) --silent $(OUT_FILE) CONFIGURATION=$(CONFIGURATION) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "ThirdPartyComponents/OSI-MAC-Prefixes Build $(OUT_FILE)...done"

clean clobber:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "ThirdPartyComponents/OSI-MAC-Prefixes $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	@rm -f $(OUT_FILE)

$(OUT_FILE):
	@MAKE_INDENT_LEVEL=$(MAKE_INDENT_LEVEL) $(StroikaRoot)ScriptsLib/WebGet $(OUT_FILE) $(FETCHURLS)
