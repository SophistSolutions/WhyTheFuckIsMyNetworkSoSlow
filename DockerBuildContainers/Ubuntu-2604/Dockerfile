# Start with basic set of tools from Stroika build environment
FROM ${BASE_IMAGE:-sophistsolutionsinc/stroika-buildvm-ubuntu2604-small:v3}

ARG INCLUDE_OPENSSH=false
ARG INCLUDE_HANDY_DEV_TOOLS=false

# anytime running apt-get intall, good idea to make sure packages downloaded up to date
# (not sure why apt-get upgrade needed, but since around 2020-09-07 needed to make following installer work)
RUN apt-get update && apt-get upgrade -y

# each distro's default g++, and ARM cross g++ - what the top Makefile's configurations use
RUN apt-get install -y g++ g++-arm-linux-gnueabihf

# NPM: For html code - nodesource's Node, as Stroika's images use
# Instructions from https://github.com/nodesource/distributions#readme
RUN \
    apt-get install -y ca-certificates curl gnupg; \
    mkdir -p /etc/apt/keyrings; \
    curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key | gpg --dearmor -o /etc/apt/keyrings/nodesource.gpg

ARG NODE_MAJOR=26
RUN echo "deb [signed-by=/etc/apt/keyrings/nodesource.gpg] https://deb.nodesource.com/node_${NODE_MAJOR}.x nodistro main" | sudo tee /etc/apt/sources.list.d/nodesource.list
RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y nodejs

ENV INCLUDE_OPENSSH=$INCLUDE_OPENSSH
RUN if [ "${INCLUDE_OPENSSH}" = true ]; then apt-get install -y openssh-server; fi
ENV INCLUDE_HANDY_DEV_TOOLS=$INCLUDE_HANDY_DEV_TOOLS
RUN if [ "${INCLUDE_HANDY_DEV_TOOLS}" = true ]; then apt-get install -y clang-format curl vim apt-file iproute2 lldb; fi

RUN rm -f /Getting-Started-With-Stroika.md
ADD Shared-Files/Getting-Started.md /
RUN sed -i -e 's/Getting-Started-With-Stroika/Getting-Started/g' $HOME/.bash_profile

### NOTE TO RUN
###     docker run -v /Sandbox:/Sandbox -it sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2604
###     docker run -it sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2604
###     (to run a WTF build on the host network, side by side with an installed service: make run-test-container)
