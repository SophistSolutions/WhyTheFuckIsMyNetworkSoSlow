SHELL=/bin/bash

all:
	mkdir -p ../IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow
	rm -f ../IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/Makefile
	ln -s `pwd`/Projects/Unix/Makefile ../IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/Makefile
	$(MAKE) --directory ../IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow all

clean:
	#$(MAKE) --directory ../IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow clean
	rm -f ../IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow

format-code:
	@../ThirdPartyComponents/Stroika/Stroika/ScriptsLib/FormatCode.sh Sources *.h *.inl *.cpp
