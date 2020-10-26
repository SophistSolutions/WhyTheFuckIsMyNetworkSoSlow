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

rm -rf $ARTIFACTS_DIR
mkdir -p $ARTIFACTS_DIR
echo "Output to $ARTIFACTS_DIR"


function runWinBld {
    cfg=$1
    branch=$2
    jobsFlag=$3
    containerName=$4
    containerImage=$5

    STARTAT_INT=$(date +%s)
    STARTAT=`date`;
    echo ">>> Creating Windows Build Container (startat = $STARTAT, containerName=$containerName)"
    echo ">>> branch=$branch, and cfg=$cfg, jobsFlag=$jobsFlag"
    docker stop $containerName
    docker rm $containerName
    docker run -itd --name $containerName $containerImage
    docker exec $containerName cmd /C "git clone --branch $branch --recurse-submodules $GIT_REPO WTFDev"
    docker exec $containerName cmd /C "cd WTFDev && make default-configurations"

    echo ">>> Starting $cfg build"
    docker exec $containerName cmd /C "cd WTFDev && make CONFIGURATION=$cfg $jobsFlag"
    echo ">>> Extracting build artifacts"
    docker stop $containerName
    docker cp $containerName:WTFDev/Builds/$cfg/WhyTheFuckIsMyNetworkSoSlow/WhyTheFuckIsMyNetworkSoSlow-Windows-x86_64-$cfg.msi $ARTIFACTS_DIR
	TOTAL_MINUTES_SPENT=$(($(( $(date +%s) - $STARTAT_INT )) / 60))
    echo ">>> Build took $TOTAL_MINUTES_SPENT minutes"
}

function runUnixBld {
    cfg=$1
    branch=$2
    jobsFlag=$3
    containerName=$4
    containerImage=$5

    STARTAT_INT=$(date +%s)
    STARTAT=`date`;
    echo ">>> Creating Unix Build Container (startat = $STARTAT, containerName=$containerName)"
    echo ">>> branch=$branch, and cfg=$cfg, jobsFlag=$jobsFlag"

    ssh $UNIX_BUILD_SSHPREFIX "docker stop $containerName; docker rm $containerName"

    ssh $UNIX_BUILD_SSHPREFIX docker run --tty --detach --name $containerName $containerImage
    ssh $UNIX_BUILD_SSHPREFIX docker exec $containerName git clone --branch $branch --recurse-submodules $GIT_REPO
    ssh $UNIX_BUILD_SSHPREFIX docker exec --workdir /WhyTheFuckIsMyNetworkSoSlow $containerName make build-root
    ssh $UNIX_BUILD_SSHPREFIX docker exec --workdir /WhyTheFuckIsMyNetworkSoSlow/ThirdPartyComponents/Stroika/StroikaRoot $containerName './configure Release --apply-default-release-flags --compiler-driver g++-8'
    ssh $UNIX_BUILD_SSHPREFIX docker exec --workdir /WhyTheFuckIsMyNetworkSoSlow $containerName "bash -c \"time make all $jobsFlag\""
    ssh $UNIX_BUILD_SSHPREFIX docker cp $containerName:/WhyTheFuckIsMyNetworkSoSlow/Builds/$cfg/WhyTheFuckIsMyNetworkSoSlow/whythefuckismynetworksoslow-1.0d8x.Linux.x86_64.deb /tmp
    scp $UNIX_BUILD_SSHPREFIX:/tmp/whythefuckismynetworksoslow-1.0d8x.Linux.x86_64.deb $ARTIFACTS_DIR

	TOTAL_MINUTES_SPENT=$(($(( $(date +%s) - $STARTAT_INT )) / 60))
    echo ">>> Build took $TOTAL_MINUTES_SPENT minutes"
}

runWinBld Release-U-64 v1-Dev -j8 WTF_Win_Build sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19:latest
runUnixBld Release v1-Dev -j10 wtfBuildUbuntux64 sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-1804

