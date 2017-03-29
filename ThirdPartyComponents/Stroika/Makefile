IntermediateFiles=		../../IntermediateFiles/


STROIKA_CONFIG_PARAMS_COMMON=

STROIKA_CONFIG_PARAMS_DEBUG=--apply-default-debug-flags
STROIKA_CONFIG_PARAMS_RELEASE=--apply-default-release-flags


all: StroikaRoot/ConfigurationFiles
	@echo "Building Stroika CONFIGURATION=$(CONFIGURATION)..."
	@$(MAKE) --directory=StroikaRoot --no-print-directory libraries CONFIGURATION=$(CONFIGURATION)
	@echo "Building Stroika CONFIGURATION=$(CONFIGURATION)...done"

clobber:
	-@$(MAKE) --directory=StroikaRoot --no-print-directory --silent clobber CONFIGURATION=$(CONFIGURATION)
	@rm -rf StroikaRoot/ConfigurationFiles StroikaRoot/Builds StroikaRoot/IntermediateFiles
	@rm -rf ../../ConfigurationFiles

configurations:	StroikaRoot/ConfigurationFiles

StroikaRoot/ConfigurationFiles:
	@echo Configuring...
	@cd StroikaRoot && ./ScriptsLib/MakeBuildRoot.sh ../../../
	@if [ `uname` = "Darwin" ] ; then\
		(cd StroikaRoot && ./configure Debug $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));\
		(cd StroikaRoot && ./configure Release $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));\
	elif [ `uname -o` = "Cygwin" ] ; then\
		make --directory=StroikaRoot --no-print-directory  default-configurations;\
	else\
		(cd StroikaRoot && ./configure Debug $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));\
		(cd StroikaRoot && ./configure Release $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));\
	fi
	@echo Configuring...done
