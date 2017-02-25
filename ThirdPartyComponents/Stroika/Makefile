IntermediateFiles=		../../IntermediateFiles/


STROIKA_CONFIG_PARAMS_COMMON=


all: Stroika/ConfigurationFiles
	@echo "Building Stroika CONFIGURATION=$(CONFIGURATION)..."
	@$(MAKE) --directory=Stroika --no-print-directory libraries CONFIGURATION=$(CONFIGURATION)
	@echo "Building Stroika CONFIGURATION=$(CONFIGURATION)...done"

clobber:
	$(MAKE) --directory=Stroika --no-print-directory clobber CONFIGURATION=$(CONFIGURATION)
	rm -rf Stroika/ConfigurationFiles
	rm -rf ../../ConfigurationFiles

Stroika/ConfigurationFiles:
	@echo Configuring...
	@cd Stroika && ./ScriptsLib/MakeBuildRoot.sh ../../../
	@if [ `uname` = "Darwin" ] ; then\
		(cd Stroika && ./configure Debug $(STROIKA_CONFIG_PARAMS_COMMON));\
		(cd Stroika && ./configure Release $(STROIKA_CONFIG_PARAMS_COMMON));\
	elif [ `uname -o` = "Cygwin" ] ; then\
		make --directory=Stroika --no-print-directory  default-configurations;\
	else\
		(cd Stroika && ./configure Debug $(STROIKA_CONFIG_PARAMS_COMMON));\
		(cd Stroika && ./configure Release $(STROIKA_CONFIG_PARAMS_COMMON));\
	fi
	@echo Configuring...done
