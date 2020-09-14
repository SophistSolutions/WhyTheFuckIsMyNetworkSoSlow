# Recipies to invoke docker from my windows box to build installers

## RECPIPE FOR BUILDING Linux X64 Ubuntu 1804 INSTALLER

`````bash
        ssh lewis@hercules "docker stop wtfBuildUbuntux64; docker rm wtfBuildUbuntux64"

        ssh lewis@hercules docker run --tty --detach --name wtfBuildUbuntux64 sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804
        ssh lewis@hercules docker exec wtfBuildUbuntux64 git clone --branch v1-Dev --recurse-submodules https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow.git
        ssh lewis@hercules docker exec --workdir /WhyTheFuckIsMyNetworkSoSlow wtfBuildUbuntux64 make build-root
        ssh lewis@hercules 'docker exec --workdir /WhyTheFuckIsMyNetworkSoSlow/ThirdPartyComponents/Stroika/StroikaRoot wtfBuildUbuntux64 ./configure Release --apply-default-release-flags --compiler-driver g++-8'
        ssh lewis@hercules 'docker exec --workdir /WhyTheFuckIsMyNetworkSoSlow wtfBuildUbuntux64 bash -c "time make all -j10"'
        ssh lewis@hercules 'docker cp wtfBuildUbuntux64:/WhyTheFuckIsMyNetworkSoSlow/Builds/Release/WhyTheFuckIsMyNetworkSoSlow/whythefuckismynetworksoslow-1.0d8x.Linux.x86_64.deb /tmp'
        scp lewis@hercules:/tmp/whythefuckismynetworksoslow-1.0d8x.Linux.x86_64.deb .
````

## RECPIPE FOR BUILDING Linux Ubuntu 1804 RaspberryPi INSTALLER

````bash
        ssh lewis@hercules "docker stop wtfBuildUbuntux64; docker rm wtfBuildUbuntux64"

        ssh lewis@hercules docker run --tty --detach --name wtfBuildUbuntux64 sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804
        ssh lewis@hercules docker exec wtfBuildUbuntux64 git clone --branch v1-Dev --recurse-submodules https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow.git
        ssh lewis@hercules docker exec --workdir /WhyTheFuckIsMyNetworkSoSlow wtfBuildUbuntux64 make build-root
        ssh lewis@hercules 'docker exec --workdir /WhyTheFuckIsMyNetworkSoSlow/ThirdPartyComponents/Stroika/StroikaRoot wtfBuildUbuntux64 ./configure raspberrypi-release --apply-default-release-flags --compiler-driver arm-linux-gnueabihf-g++-8 --cross-compiling true'
        ssh lewis@hercules 'docker exec --workdir /WhyTheFuckIsMyNetworkSoSlow wtfBuildUbuntux64 bash -c "time make all -j10"'
        ssh lewis@hercules 'docker cp wtfBuildUbuntux64:/WhyTheFuckIsMyNetworkSoSlow/Builds/raspberrypi-release/WhyTheFuckIsMyNetworkSoSlow/whythefuckismynetworksoslow-1.0d8x.Linux.armhf.deb /tmp'
        scp lewis@hercules:/tmp/whythefuckismynetworksoslow-1.0d8x.Linux.armhf.deb .
````


## RECPIPE FOR BUILDING WINDOWS INSTALLER

~~~bash
        docker stop wtfBuildWindows; docker rm wtfBuildWindows
        docker run --tty --detach --name wtfBuildWindows sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19
        docker exec wtfBuildWindows git clone --branch v1-Dev --recurse-submodules https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow.git
        docker exec --workdir /WhyTheFuckIsMyNetworkSoSlow wtfBuildWindows make build-root
        docker exec --workdir /WhyTheFuckIsMyNetworkSoSlow/ThirdPartyComponents/Stroika/StroikaRoot wtfBuildWindows sh -c "./configure Release-U-64 --arch x86_64 --config-tag Windows --config-tag 64 --apply-default-release-flags"
        docker exec --workdir /WhyTheFuckIsMyNetworkSoSlow wtfBuildWindows sh -c "time make all -j8"
        docker stop wtfBuildWindows
        docker cp wtfBuildWindows:/WhyTheFuckIsMyNetworkSoSlow/Builds/Release-U-64/WhyTheFuckIsMyNetworkSoSlow/WhyTheFuckIsMyNetworkSoSlow-Windows-x86_64-Release-U-64.msi WhyTheFuckIsMyNetworkSoSlow-Windows-x86_64-Release-U-64.msi
`````
