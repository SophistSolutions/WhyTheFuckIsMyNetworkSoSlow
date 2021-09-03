# Start with basic set of tools from Stroika build environment
FROM ${BASE_IMAGE:-sophistsolutionsinc/stroika-buildvm-ubuntu2004-small}

# anytime running apt-get intall, good idea to make sure packages downloaded up to date
# (not sure why apt-get upgrade needed, but since around 2020-09-07 needed to make following installer work)
RUN apt-get update && apt-get upgrade -y

RUN apt-get install -y g++-10

# Cross-compile for ARM
RUN apt-get install -y g++-10-arm-linux-gnueabihf

# For html code
RUN apt-get install -y nodejs npm

# Because Ubuntu1804 node version not working
RUN apt-get install -y curl
RUN curl -sL https://deb.nodesource.com/setup_10.x -o nodesource_setup.sh
RUN bash nodesource_setup.sh
RUN apt-get install -y nodejs

RUN rm -f /Getting-Started-With-Stroika.md
ADD Shared-Files/Getting-Started.md /

CMD /bin/bash

### NOTE TO RUN
###     docker run -v /Sandbox:/Sandbox -it sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004
###     docker run -it sophistsolutionsinc/whythefuckismynetworksoslow-ubuntu-2004
