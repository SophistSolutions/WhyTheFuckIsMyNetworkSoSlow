#!/bin/bash

#
# Not fully general for building, as contains lots of tweaks specific to what configs I build/distribute
# and the layout of build machines I work with (nice if I could just use travisci/circleci but those too painful still)
#   --LGP 2020-10-26
#

echo Doing release builds

ARTIFACTS_DIR=Build-ARTIFACTS/
UNIX_BUILD_SSHPREFIX=lewis@hercules
GIT_REPO=https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow.git
STROIKA_REL_ROOT=ThirdPartyComponents/Stroika/StroikaRoot/
VERSION=`$STROIKA_REL_ROOT/ScriptsLib/ExtractVersionInformation VERSION FullVersionString`

echo ">>> Build Artifacts Output to $ARTIFACTS_DIR"
echo ">>> VERSION=$VERSION"
echo ""

rm -rf $ARTIFACTS_DIR
mkdir -p $ARTIFACTS_DIR


function runWinBld {
    cfg=$1
    cfgDef=$2
    branch=$3
    jobsFlag=$4
    containerName=$5
    containerImage=$6

    STARTAT_INT=$(date +%s)
    STARTAT=`date`;
    echo ">>> Creating Windows Build Container (startat = $STARTAT, containerName=$containerName)"
    echo ">>> branch=$branch, and jobsFlag=$jobsFlag"
    echo ">>> cfg=$cfg, cfgDef=$cfgDef"

    docker stop $containerName; docker rm $containerName
    docker run -itd --name $containerName $containerImage
    docker exec $containerName cmd /C "git clone --branch $branch --recurse-submodules $GIT_REPO WTFDev"
    docker exec $containerName cmd /C "cd WTFDev && cd $STROIKA_REL_ROOT && ./configure $cfg $cfgDef"

    arch=`$STROIKA_REL_ROOT/ScriptsLib/GetConfigurationParameter $cfg ARCH`
    echo ">>> Starting $cfg build (arch=$arch)"
    docker exec $containerName cmd /C "cd WTFDev && make CONFIGURATION=$cfg $jobsFlag"
    echo ">>> Extracting build artifacts"
    docker stop $containerName
    docker cp $containerName:WTFDev/Builds/$cfg/WhyTheFuckIsMyNetworkSoSlow/WhyTheFuckIsMyNetworkSoSlow-Windows-$arch-$cfg.msi $ARTIFACTS_DIR
	TOTAL_MINUTES_SPENT=$(($(( $(date +%s) - $STARTAT_INT )) / 60))
    echo ">>> Build took $TOTAL_MINUTES_SPENT minutes"
}

function runUnixBld {
    cfg=$1
    cfgDef=$2
    branch=$3
    jobsFlag=$4
    containerName=$5
    containerImage=$6

    STARTAT_INT=$(date +%s)
    STARTAT=`date`;
    echo ">>> Creating Unix Build Container (startat = $STARTAT, containerName=$containerName)"
    echo ">>> branch=$branch, and jobsFlag=$jobsFlag"
    echo ">>> cfg=$cfg, cfgDef=$cfgDef"

    ssh $UNIX_BUILD_SSHPREFIX "docker stop $containerName; docker rm $containerName"

    ssh $UNIX_BUILD_SSHPREFIX docker run --tty --detach --name $containerName $containerImage
    ssh $UNIX_BUILD_SSHPREFIX docker exec $containerName git clone --branch $branch --recurse-submodules $GIT_REPO WTFDev
    ssh $UNIX_BUILD_SSHPREFIX docker exec --workdir /WTFDev $containerName make build-root
    ssh $UNIX_BUILD_SSHPREFIX docker exec --workdir /WTFDev/ThirdPartyComponents/Stroika/StroikaRoot $containerName "./configure $cfg $cfgDef"
    arch=`ssh $UNIX_BUILD_SSHPREFIX docker exec --workdir /WTFDev/ThirdPartyComponents/Stroika/StroikaRoot $containerName ScriptsLib/GetConfigurationParameter $cfg ARCH`
    echo ">>> Starting $cfg build (arch=$arch)"
    ssh $UNIX_BUILD_SSHPREFIX docker exec --workdir /WTFDev $containerName "bash -c \"time make all $jobsFlag\""
    echo ">>> Extracting build artifacts"   ## sadly cannot use wildcards in docker cp as of 2020-10-27
    ssh $UNIX_BUILD_SSHPREFIX docker cp $containerName:/WTFDev/Builds/$cfg/WhyTheFuckIsMyNetworkSoSlow/whythefuckismynetworksoslow-$VERSION.Linux.$arch.deb /tmp 2> /dev/null
    ssh $UNIX_BUILD_SSHPREFIX docker cp $containerName:/WTFDev/Builds/$cfg/WhyTheFuckIsMyNetworkSoSlow/whythefuckismynetworksoslow-$VERSION.Linux.$arch.rpm /tmp 2> /dev/null
    scp $UNIX_BUILD_SSHPREFIX:/tmp/whythefuckismynetworksoslow-$VERSION.Linux.$arch.* $ARTIFACTS_DIR

	TOTAL_MINUTES_SPENT=$(($(( $(date +%s) - $STARTAT_INT )) / 60))
    echo ">>> Build took $TOTAL_MINUTES_SPENT minutes"
}

runWinBld Release-U-64 "--apply-default-release-flags --arch x86_64" v1-Dev -j8 WTF_Win_Build sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19:latest
runUnixBld Release "--apply-default-release-flags --compiler-driver g++-8" v1-Dev -j10 wtfBuildUbuntux64 sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804

