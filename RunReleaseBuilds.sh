#!/bin/bash

echo Doing release builds

ARTIFACTS_DIR=Build-ARTIFACTS
rm -rf $ARTIFACTS_DIR
mkdir $ARTIFACTS_DIR
echo "Output to $ARTIFACTS_DIR"


echo Creating Windows Build Container
docker stop WTF_Win_Build
docker rm WTF_Win_Build
docker run -itd --name WTF_Win_Build sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19:latest
docker exec WTF_Win_Build cmd /C "git clone --branch v1-Dev --recurse-submodules https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow.git WTFDev"
docker exec WTF_Win_Build cmd /C "cd WTFDev && make default-configurations"

CONFIGURATION=Release-U-64
echo "*******Running 64-bit build**********"
docker exec WTF_Win_Build cmd /C "cd WTFDev && make CONFIGURATION=$CONFIGURATION -j4"
echo "*******Running 64-bit build**********: extracting build artifacts"
docker stop WTF_Win_Build
docker cp WTF_Win_Build:WTFDev/Builds/$CONFIGURATION/WhyTheFuckIsMyNetworkSoSlow/WhyTheFuckIsMyNetworkSoSlow-Windows-x86_64-$CONFIGURATION.msi $ARTIFACTS_DIR

