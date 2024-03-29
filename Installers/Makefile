export TOP_ROOT=$(abspath ../)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifneq ($(CONFIGURATION),)
	include $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/Configuration.mk
endif

include $(StroikaRoot)ScriptsLib/Makefile-Common.mk

BASIC_NAME_LC=whythefuckismynetworksoslow
BASIC_NAME=WhyTheFuckIsMyNetworkSoSlow
OUTDIR_NAME=WhyTheFuckIsMyNetworkSoSlow
BASIC_BACKEND_NAME=$(BASIC_NAME)-Backend


ObjDir				=	$(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/$(OUTDIR_NAME)/

include $(StroikaRoot)ScriptsLib/SharedBuildRules-Default.mk
include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk


TARGET_OUT_DIR=			$(StroikaRoot)Builds/$(CONFIGURATION)/$(OUTDIR_NAME)/

BUILT_SERVICE_EXE				=	$(TARGET_OUT_DIR)WhyTheFuckIsMyNetworkSoSlow-Backend${EXE_SUFFIX}
BUILT_DATA_DIR					=	$(TARGET_OUT_DIR)data/
BUILT_HTML_DIR					=	$(TARGET_OUT_DIR)html/

VERSION_FILE_IN=$(TOP_ROOT)VERSION



MAJOR_DOT_MINOR_VERSION=$(shell $(StroikaRoot)/ScriptsLib/ExtractVersionInformation $(VERSION_FILE_IN) Major.Minor)
INSTALLER_RELEASE_TAG=$(shell $(StroikaRoot)/ScriptsLib/ExtractVersionInformation $(VERSION_FILE_IN) DecoratedStageInfo)
FULLVERSION=$(shell $(StroikaRoot)/ScriptsLib/ExtractVersionInformation $(VERSION_FILE_IN) FullVersionString)
#INSTALLER_PLATFORM_TAG?=$(shell uname -m)
INSTALLER_PLATFORM_TAG?=$(ARCH)
INSTALLER_OS_TAG?=$(shell uname -s)
ifeq (CYGWIN,$(findstring CYGWIN,$(INSTALLER_OS_TAG)))
INSTALLER_OS_TAG=Windows
endif

ifeq (Release,$(CONFIGURATION))
CFG_IFNOT_RELEASE_NAME_APPENDAGE=
else
CFG_IFNOT_RELEASE_NAME_APPENDAGE=-$(CONFIGURATION)
endif
TARGET_OUT_INSTALLER_DEB=$(TARGET_OUT_DIR)$(BASIC_NAME_LC)-$(INSTALLER_OS_TAG)$(CFG_IFNOT_RELEASE_NAME_APPENDAGE)-$(DEB_VERSION)_$(INSTALLER_PLATFORM_TAG).deb
TARGET_OUT_INSTALLER_RPM=$(TARGET_OUT_DIR)$(BASIC_NAME_LC)-$(INSTALLER_OS_TAG)$(CFG_IFNOT_RELEASE_NAME_APPENDAGE)-$(RPM_VERSION).$(INSTALLER_PLATFORM_TAG).rpm
TARGET_OUT_INSTALLER_WIX=$(TARGET_OUT_DIR)$(BASIC_NAME)-$(INSTALLER_OS_TAG)$(CFG_IFNOT_RELEASE_NAME_APPENDAGE)-$(FULLVERSION)-$(ARCH).msi

TARGET_OUT_INSTALLERS=
ifeq ($(shell test -x "`command -v dpkg 2>/dev/null`"; echo $$?),0)
TARGET_OUT_INSTALLERS+=${TARGET_OUT_INSTALLER_DEB}
endif
ifeq ($(shell test -x "`command -v rpmbuild 2>/dev/null`"; echo $$?),0)
TARGET_OUT_INSTALLERS+=${TARGET_OUT_INSTALLER_RPM}
endif
ifneq ($(qFeatureFlag_WIX),no)
TARGET_OUT_INSTALLERS+=${TARGET_OUT_INSTALLER_WIX}
endif

.PHONY:  all installers


all:
ifeq ($(CONFIGURATION),)
	$(error Cannot make installers without a configuration argument)
endif
ifneq ($(TARGET_OUT_INSTALLERS),)
	@$(MAKE) --silent $(TARGET_OUT_INSTALLERS)
endif


installers:
ifeq ($(CONFIGURATION),)
	$(error Cannot make installers without a configuration argument)
endif
ifneq ($(TARGET_OUT_INSTALLERS),)
	@$(MAKE) --no-print-directory $(TARGET_OUT_INSTALLERS)
endif

installer-deb:
ifeq ($(CONFIGURATION),)
	$(error Cannot make installers without a configuration argument)
endif
	@$(MAKE) --no-print-directory $(TARGET_OUT_INSTALLER_DEB)

installer-rpm:
ifeq ($(CONFIGURATION),)
	$(error Cannot make installers without a configuration argument)
endif
	@$(MAKE) --no-print-directory $(TARGET_OUT_INSTALLER_RPM)

installer-wix:
ifeq ($(CONFIGURATION),)
	$(error Cannot make installers without a configuration argument)
endif
	@$(MAKE) --no-print-directory $(TARGET_OUT_INSTALLER_WIX)


#The 'deb' package creator requires the directory have particular permissions. on WSL (windows subsystem for linux)
#using the /mnt filesystem, the permissions don't work. So use /tmp for these. But that means creating the directories
#in a temp location (already emppty) and deleting it at the end (bit of a PITA if you want to review the data, but hten just
#comment out the delete line)
#
# NOTE - if you mount the drivefs filesystem with the option "metadata" permissions work, but until msft makes this the default
# just comment out...
# disable compute of workdir (mktemp) on windows / cygwin since not needed and fails and no need to debug now.
ifneq (VisualStudio,$(findstring VisualStudio,$(ProjectPlatformSubdir)))
#DEB_WORKDIR=$(ObjDir)DEB_TMP/
DEB_WORKDIR:=$(shell mktemp -d)/
endif
DEB_VERSION=$(MAJOR_DOT_MINOR_VERSION)$(INSTALLER_RELEASE_TAG)
DEB_BUILD_OUT=$(ObjDir)DEB-Installer-Build.Out
$(TARGET_OUT_INSTALLER_DEB): $(BUILT_SERVICE_EXE) Makefile ${BUILT_DATA_DIR} ${BUILT_HTML_DIR}
	@$(StroikaRoot)/ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "Building DEB installer: "
	@$(StroikaRoot)/ScriptsLib/PrintProgressLine $$(($(MAKE_INDENT_LEVEL)+1)) `$(StroikaRoot)ScriptsLib/SubstituteBackVariables "temp files: $(DEB_WORKDIR); log: $(DEB_BUILD_OUT)"`
ifeq ($(CONFIGURATION),)
	$(error Cannot make installer-deb without a configuration argument)
endif
	@#NB: This installs control scripts for systemd, so assumes ubuntu 15 or later --LGP 2017-07-29
	@#Make directories to be created by the installer
	@mkdir -p $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/DEBIAN
	@mkdir -p $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/opt/$(BASIC_NAME)/
	@mkdir -p $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/usr/lib/systemd/system
	@mkdir -p $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/var/opt/$(BASIC_NAME)/
	@chmod 755 -R $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)
	
	@echo "Version: $(DEB_VERSION)" >> $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/DEBIAN/control
	@cat $(BASIC_NAME_LC).control.static >> $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/DEBIAN/control
	@echo "Architecture: `$(StroikaRoot)ScriptsLib/MapArch2DebFileArchitecture ${ARCH}`" >> $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/DEBIAN/control

	@echo "command -v systemd > /dev/null 2>&1; if [ \$$? -eq 0 ] ; then systemctl enable $(BASIC_NAME_LC) ; else echo \"Skipping systemd because not supported on target system\"; fi" >> $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/DEBIAN/postinst
	@echo "command -v systemd > /dev/null 2>&1; if [ \$$? -eq 0 ] ; then systemctl start $(BASIC_NAME_LC) ; fi" >> $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/DEBIAN/postinst
	@chmod 755 $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/DEBIAN/postinst

	@echo "command -v systemd > /dev/null 2>&1; if [ \$$? -eq 0 ] ; then ((systemctl stop $(BASIC_NAME_LC)) || true); fi" >> $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/DEBIAN/prerm
	@echo "command -v systemd > /dev/null 2>&1; if [ \$$? -eq 0 ] ; then ((systemctl disable $(BASIC_NAME_LC)) || true); fi" >> $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/DEBIAN/prerm
	@chmod 755 $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/DEBIAN/prerm

	@#List files to actually install and where
ifeq ($(IncludeDebugSymbolsInExecutables),1)
	@install -m 755 $(BUILT_SERVICE_EXE) $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/opt/$(BASIC_NAME)/$(BASIC_BACKEND_NAME)
else
	@install -m 755 --strip --strip-program=$(STRIP) $(BUILT_SERVICE_EXE) $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/opt/$(BASIC_NAME)/$(BASIC_BACKEND_NAME)
endif
	@install -m 644 $(BASIC_NAME_LC).service $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/usr/lib/systemd/system/$(BASIC_NAME_LC).service
	@cp -r $(BUILT_DATA_DIR) $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/opt/$(BASIC_NAME)/data
	@cp -r $(BUILT_HTML_DIR) $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION)/opt/$(BASIC_NAME)/html

	@dpkg --build $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION) 2>&1 > $(DEB_BUILD_OUT)
	@cp $(DEB_WORKDIR)$(BASIC_NAME_LC)-$(DEB_VERSION).deb $(TARGET_OUT_INSTALLER_DEB)
	
	@rm -rf $(DEB_WORKDIR)
	@$(StroikaRoot)/ScriptsLib/PrintProgressLine $$(($(MAKE_INDENT_LEVEL)+1)) "Built: "`$(StroikaRoot)ScriptsLib/SubstituteBackVariables "$(TARGET_OUT_INSTALLER_DEB)"`


RPM_WORKDIR=$(ObjDir)RPM_TMP/
RPM_VERSION=$(MAJOR_DOT_MINOR_VERSION)$(INSTALLER_RELEASE_TAG)
RPM_BUILD_OUT=$(ObjDir)RPM-Installer-Build.Out
$(TARGET_OUT_INSTALLER_RPM): $(BUILT_SERVICE_EXE) Makefile ${BUILT_DATA_DIR} ${BUILT_HTML_DIR}
	@$(StroikaRoot)/ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "Building RPM installer: "
	@$(StroikaRoot)/ScriptsLib/PrintProgressLine $$(($(MAKE_INDENT_LEVEL)+1)) `$(StroikaRoot)ScriptsLib/SubstituteBackVariables "temp files: $(RPM_WORKDIR); log: $(RPM_BUILD_OUT)"`"..."
ifeq ($(CONFIGURATION),)
	$(error Cannot make installer-rpm without a configuration argument)
endif
	@rm -rf $(RPM_WORKDIR)
	@#Make directories to be created by the installer
	@mkdir -p $(RPM_WORKDIR)RPMS $(RPM_WORKDIR)SRPMS $(RPM_WORKDIR)BUILD $(RPM_WORKDIR)SOURCES $(RPM_WORKDIR)SPECS $(RPM_WORKDIR)tmp
	@mkdir -p $(RPM_WORKDIR)$(BASIC_NAME_LC)-$(MAJOR_DOT_MINOR_VERSION)/opt/$(BASIC_NAME)/
	@mkdir -p $(RPM_WORKDIR)$(BASIC_NAME_LC)-$(MAJOR_DOT_MINOR_VERSION)/usr/lib/systemd/system
	@mkdir -p $(RPM_WORKDIR)$(BASIC_NAME_LC)-$(MAJOR_DOT_MINOR_VERSION)/var/opt/$(BASIC_NAME)/

ifeq ($(IncludeDebugSymbolsInExecutables),1)
	@install -m 755 $(BUILT_SERVICE_EXE) $(RPM_WORKDIR)/$(BASIC_NAME_LC)-$(MAJOR_DOT_MINOR_VERSION)/opt/$(BASIC_NAME)/$(BASIC_BACKEND_NAME)
else
	@install -m 755 --strip --strip-program=$(STRIP) $(BUILT_SERVICE_EXE) $(RPM_WORKDIR)/$(BASIC_NAME_LC)-$(MAJOR_DOT_MINOR_VERSION)/opt/$(BASIC_NAME)/$(BASIC_BACKEND_NAME)
endif
	@install -m 644 $(BASIC_NAME_LC).service $(RPM_WORKDIR)$(BASIC_NAME_LC)-$(MAJOR_DOT_MINOR_VERSION)/usr/lib/systemd/system/$(BASIC_NAME_LC).service
	@cp -r $(BUILT_DATA_DIR) $(RPM_WORKDIR)$(BASIC_NAME_LC)-$(MAJOR_DOT_MINOR_VERSION)/opt/$(BASIC_NAME)/data
	@cp -r $(BUILT_HTML_DIR) $(RPM_WORKDIR)$(BASIC_NAME_LC)-$(MAJOR_DOT_MINOR_VERSION)/opt/$(BASIC_NAME)/html

	@cd $(RPM_WORKDIR) && tar -zcf $(BASIC_NAME_LC)-$(MAJOR_DOT_MINOR_VERSION).tar.gz $(BASIC_NAME_LC)-$(MAJOR_DOT_MINOR_VERSION)/
	@cd $(RPM_WORKDIR) && cp $(BASIC_NAME_LC)-$(MAJOR_DOT_MINOR_VERSION).tar.gz SOURCES/
	@echo "%define _topdir "$(RPM_WORKDIR) > $(RPM_WORKDIR)/SPECS/$(BASIC_NAME_LC).spec
	@echo "Version: $(MAJOR_DOT_MINOR_VERSION)" >> $(RPM_WORKDIR)SPECS/$(BASIC_NAME_LC).spec
	@echo "Release: $(INSTALLER_RELEASE_TAG)" >> $(RPM_WORKDIR)SPECS/$(BASIC_NAME_LC).spec
	@cat $(BASIC_NAME_LC).rpm.spec >> $(RPM_WORKDIR)SPECS/$(BASIC_NAME_LC).spec
	@cd $(RPM_WORKDIR) && rpmbuild -ba --quiet SPECS/$(BASIC_NAME_LC).spec > $(RPM_BUILD_OUT) 2>&1

	@cp $(RPM_WORKDIR)RPMS/*/*.rpm $(TARGET_OUT_INSTALLER_RPM)
	@$(StroikaRoot)/ScriptsLib/PrintProgressLine $$(($(MAKE_INDENT_LEVEL)+1)) "Built: "`$(StroikaRoot)ScriptsLib/SubstituteBackVariables "$(TARGET_OUT_INSTALLER_RPM)"`


WIX_WORKDIR=$(ObjDir)WIX_TMP/
WIX_OUT_OBJ_DIR=$(WIX_WORKDIR)
WIX_OUT_OBJ_DIR_WIN=$(shell cygpath --mixed $(WIX_OUT_OBJ_DIR))
WIX_BUILD_OUT=$(ObjDir)WIX-Installer-Build.Out
WIX_VERSION=$(MAJOR_DOT_MINOR_VERSION)$(INSTALLER_RELEASE_TAG)
WIX_4DOT_VERSION=$(shell $(StroikaRoot)/ScriptsLib/ExtractVersionInformation $(VERSION_FILE_IN) Win32FourDot)
ifeq ($(qFeatureFlag_WIX),use)
WIX_PATH_PREFIX=PATH="$(StroikaRoot)ThirdPartyComponents/WIX/CURRENT/:$(PATH)"
else
WIX_PATH_PREFIX=
endif
#CANDLE uses x64 not x86_64
CANDLE_ARCH=$(ARCH)
ifeq ($(ARCH),x86_64)
CANDLE_ARCH=x64
endif
TARGET_OUT_INSTALLER_WIX_WINDOWSPATH=$(shell cygpath -w $(TARGET_OUT_INSTALLER_WIX))
DATADIR_WINDOWSPATH=$(shell cygpath --mixed $(WIX_WORKDIR)data)
HTMLDIR_WINDOWSPATH=$(shell cygpath --mixed $(WIX_WORKDIR)html)
WIXDATAFRAGFILE_WINDOWPATH=$(shell cygpath --mixed $(WIX_WORKDIR)data.wxs)
WIXHTMLFRAGFILE_WINDOWPATH=$(shell cygpath --mixed $(WIX_WORKDIR)html.wxs)
$(TARGET_OUT_INSTALLER_WIX): $(BUILT_SERVICE_EXE) Makefile ${BUILT_DATA_DIR} ${BUILT_HTML_DIR}
	@$(StroikaRoot)/ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "Building WIX installer: "
	@$(StroikaRoot)/ScriptsLib/PrintProgressLine $$(($(MAKE_INDENT_LEVEL)+1)) `$(StroikaRoot)ScriptsLib/SubstituteBackVariables "temp files: $(WIX_WORKDIR); log: $(WIX_BUILD_OUT)"`"..."
ifeq ($(CONFIGURATION),)
	$(error Cannot make WIX installer without a configuration argument)
endif
	@rm -rf $(WIX_WORKDIR) $(WIX_BUILD_OUT)
	@#Make directories to be created by the installer
	@mkdir -p $(WIX_WORKDIR)
	@cp $(BUILT_SERVICE_EXE) $(WIX_WORKDIR)$(BASIC_NAME)-Backend${EXE_SUFFIX} 2>&1 >> $(WIX_BUILD_OUT)
	@cp -r $(BUILT_DATA_DIR) $(WIX_WORKDIR)data
	@cp -r $(BUILT_HTML_DIR) $(WIX_WORKDIR)html
	@echo heat dir $(WIX_PATH_PREFIX) heat dir $(DATADIR_WINDOWSPATH) -nologo -o $(WIXDATAFRAGFILE_WINDOWPATH) -scom -frag -srd -sreg -gg -cg StroikaSampleService_data_comp -dr DATA_DIR_REF >> $(WIX_BUILD_OUT)
	@echo heat dir $(WIX_PATH_PREFIX) heat dir $(HTMLDIR_WINDOWSPATH) -nologo -o $(WIXHTMLFRAGFILE_WINDOWPATH) -scom -frag -srd -sreg -gg -cg StroikaSampleService_html_comp -dr HTML_DIR_REF >> $(WIX_BUILD_OUT)
	@$(WIX_PATH_PREFIX) heat dir $(DATADIR_WINDOWSPATH) -nologo -o $(WIXDATAFRAGFILE_WINDOWPATH) -scom -frag -srd -sreg -gg -cg StroikaSampleService_data_comp -dr DATA_DIR_REF
	@$(WIX_PATH_PREFIX) heat dir $(HTMLDIR_WINDOWSPATH) -nologo -o $(WIXHTMLFRAGFILE_WINDOWPATH) -scom -frag -srd -sreg -gg -cg StroikaSampleService_html_comp -dr HTML_DIR_REF
	@echo WIX_PATH_PREFIX = $(WIX_PATH_PREFIX) >> $(WIX_BUILD_OUT)
	@echo candle.exe -v -arch $(CANDLE_ARCH) -dARCH=$(ARCH) -dCONFIGURATION=$(CONFIGURATION) -dDISPLAY_FULL_VERSION=$(WIX_VERSION) -dWIX_4DOT_VERSION=$(WIX_4DOT_VERSION) Installer.wix $(WIXDATAFRAGFILE_WINDOWPATH) $(WIXHTMLFRAGFILE_WINDOWPATH) -out $(WIX_OUT_OBJ_DIR_WIN) >> $(WIX_BUILD_OUT)
	@$(WIX_PATH_PREFIX) candle.exe -v -arch $(CANDLE_ARCH) -dARCH=$(ARCH) -dCONFIGURATION=$(CONFIGURATION) -dDISPLAY_FULL_VERSION=$(WIX_VERSION) -dWIX_4DOT_VERSION=$(WIX_4DOT_VERSION) Installer.wix $(WIXDATAFRAGFILE_WINDOWPATH) $(WIXHTMLFRAGFILE_WINDOWPATH) -out $(WIX_OUT_OBJ_DIR_WIN) 2>&1 >> $(WIX_BUILD_OUT)
	@echo light.exe $(shell cygpath --mixed $(WIX_OUT_OBJ_DIR)Installer.wixobj) $(shell cygpath --mixed $(WIX_OUT_OBJ_DIR)data.wixobj) $(shell cygpath --mixed $(WIX_OUT_OBJ_DIR)html.wixobj) -b $(DATADIR_WINDOWSPATH)-b $(HTMLDIR_WINDOWSPATH) -ext WixUIExtension -cultures:en-us -loc Installer_en-us.wxl -out '$(TARGET_OUT_INSTALLER_WIX_WINDOWSPATH)'  >> $(WIX_BUILD_OUT)
	@$(WIX_PATH_PREFIX) light.exe $(shell cygpath --mixed $(WIX_OUT_OBJ_DIR)Installer.wixobj) $(shell cygpath --mixed $(WIX_OUT_OBJ_DIR)data.wixobj) $(shell cygpath --mixed $(WIX_OUT_OBJ_DIR)html.wixobj) -b $(DATADIR_WINDOWSPATH) -b $(HTMLDIR_WINDOWSPATH) -ext WixUIExtension -cultures:en-us -loc Installer_en-us.wxl -out '$(TARGET_OUT_INSTALLER_WIX_WINDOWSPATH)' 2>&1 >> $(WIX_BUILD_OUT)
	@$(StroikaRoot)/ScriptsLib/PrintProgressLine $$(($(MAKE_INDENT_LEVEL)+1)) "Built: "`$(StroikaRoot)ScriptsLib/SubstituteBackVariables "$(TARGET_OUT_INSTALLER_WIX)"`
