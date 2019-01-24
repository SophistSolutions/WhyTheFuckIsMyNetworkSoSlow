# Quick start building WhyTheFuckIsMyNetworkSoSlow

- git clone https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow.git
- cd WhyTheFuckIsMyNetworkSoSlow
- git submodule update --init --recursive
- make all -j10
  {builds all configurations}

or see Makefile 'default-configurations' section to see what configuration files were created, and instead create your own.

And then

 `make CONFIGURAITON=YOUR-SELECTED-CONFIGURATION all -j10`

Or

 `make TAGS=Unix all -j10`

to build all configurations with the tag Unix
