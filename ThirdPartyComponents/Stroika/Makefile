IntermediateFiles=		../../IntermediateFiles/


STROIKA_CONFIG_PARAMS_COMMON=


all:
	$(MAKE) --directory=Stroika --no-print-directory all CONFIGURATION=$(CONFIGURATION)

configure:
	rm -rf Stroika/ConfigurationFiles;
	if [ `uname -o` = "Cygwin" ] ; then\
		make --directory=Stroika --no-print-directory  default-configurations;\
	else\
		cd Stroika && ./configure Debug $(STROIKA_CONFIG_PARAMS_COMMON);\
		cd Stroika && ./configure Release $(STROIKA_CONFIG_PARAMS_COMMON);\
	fi