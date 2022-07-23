# Start with basic set of tools from Stroika build environment
FROM ${BASE_IMAGE:-sophistsolutionsinc/stroika-buildvm-ubuntu2004-small}

ARG INCLUDE_OPENSSL=false
ARG INCLUDE_HANDY_DEV_TOOLS=false

# anytime running apt-get intall, good idea to make sure packages downloaded up to date
# (not sure why apt-get upgrade needed, but since around 2020-09-07 needed to make following installer work)
RUN apt-get update && apt-get upgrade -y

RUN apt-get install -y g++-10

# Cross-compile for ARM
RUN apt-get install -y g++-10-arm-linux-gnueabihf

# Because Ubuntu2004 node version not working with vite (requires 14.x or later, but that has version of npm that didn't seem to work with builds)
RUN apt-get install -y curl
RUN curl -sL https://deb.nodesource.com/setup_18.x | sudo -E bash -

# For html code
RUN apt-get install -y nodejs
# Somehow installing npm from apt-get (using newer nodejs) not working --LGP 2022-06-20
RUN npm install -g npm@latest

ENV INCLUDE_OPENSSL $INCLUDE_OPENSSL
RUN if [ "${INCLUDE_OPENSSL}" = true ]; then apt-get install -y openssh-server; fi
ENV INCLUDE_HANDY_DEV_TOOLS $INCLUDE_HANDY_DEV_TOOLS
RUN if [ "${INCLUDE_HANDY_DEV_TOOLS}" = true ]; then apt-get install -y clang-format curl vim apt-file iproute2 npm lldb; fi

RUN rm -f /Getting-Started-With-Stroika.md
ADD Shared-Files/Getting-Started.md /

CMD /bin/bash

### NOTE TO RUN
###     docker run -v /Sandbox:/Sandbox -it sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004
###     docker run -it sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004
