# Start with basic set of tools from Stroika build environment
FROM ${BASE_IMAGE:-sophistsolutionsinc/stroika-buildvm-ubuntu2204-small:v2.1}

ARG INCLUDE_OPENSSL=false
ARG INCLUDE_HANDY_DEV_TOOLS=false

# anytime running apt-get intall, good idea to make sure packages downloaded up to date
# (not sure why apt-get upgrade needed, but since around 2020-09-07 needed to make following installer work)
RUN apt-get update && apt-get upgrade -y

RUN apt-get install -y g++-12

# Cross-compile for ARM
RUN apt-get install -y g++-12-arm-linux-gnueabihf

# NPM: For html code
#
# Because Ubuntu2004 node version not working with vite (requires 14.x or later, but that has version of npm that didn't seem to work with builds)
# Instructions from https://github.com/nodesource/distributions#readme
RUN \
    apt-get install -y ca-certificates curl gnupg; \
    mkdir -p /etc/apt/keyrings; \
    curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key | gpg --dearmor -o /etc/apt/keyrings/nodesource.gpg

ARG NODE_MAJOR=20
RUN echo "deb [signed-by=/etc/apt/keyrings/nodesource.gpg] https://deb.nodesource.com/node_${NODE_MAJOR}.x nodistro main" | sudo tee /etc/apt/sources.list.d/nodesource.list
RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y nodejs 

ENV INCLUDE_OPENSSL $INCLUDE_OPENSSL
RUN if [ "${INCLUDE_OPENSSL}" = true ]; then apt-get install -y openssh-server; fi
ENV INCLUDE_HANDY_DEV_TOOLS $INCLUDE_HANDY_DEV_TOOLS
RUN if [ "${INCLUDE_HANDY_DEV_TOOLS}" = true ]; then apt-get install -y clang-format curl vim apt-file iproute2 lldb; fi

RUN rm -f /Getting-Started-With-Stroika.md
ADD Shared-Files/Getting-Started.md /

CMD /bin/bash

### NOTE TO RUN
###     docker run -v /Sandbox:/Sandbox -it sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004
###     docker run -it sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004
