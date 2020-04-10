# Start with basic set of tools from Stroika build environment
FROM ${BASE_IMAGE:-sophistsolutionsinc/stroika-buildvm-ubuntu1804-small}

RUN apt-get update

# Cross-compile tests for ARM
RUN apt-get install -y gcc-8-arm-linux-gnueabihf

# For html code
RUN apt-get install -y nodejs npm

CMD /bin/bash
