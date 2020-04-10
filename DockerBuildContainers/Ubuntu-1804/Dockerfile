# Start with basic set of tools from Stroika build environment
FROM ${BASE_IMAGE:-sophistsolutionsinc/stroika-buildvm-ubuntu1804-small}

# anytime running apt-get intall, good idea to make sure packages downloaded up to date
RUN apt-get update

# Cross-compile for ARM
RUN apt-get install -y gcc-8-arm-linux-gnueabihf

# For html code
RUN apt-get install -y nodejs npm

CMD /bin/bash
