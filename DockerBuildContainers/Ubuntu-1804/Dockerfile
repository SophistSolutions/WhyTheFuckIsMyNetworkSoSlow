# Start with basic set of tools from Stroika build environment
FROM ${BASE_IMAGE:-sophistsolutionsinc/stroika-buildvm-ubuntu1804-small}

# anytime running apt-get intall, good idea to make sure packages downloaded up to date
# (not sure why apt-get upgrade needed, but since around 2020-09-07 needed to make following installer work)
RUN apt-get update && apt-get upgrade -y

# Cross-compile for ARM
RUN apt-get install -y gcc-8-arm-linux-gnueabihf

# For html code
RUN apt-get install -y nodejs npm

RUN rm -f /Getting-Started-With-Stroika.md
ADD Shared-Files/Getting-Started.md /

CMD /bin/bash
