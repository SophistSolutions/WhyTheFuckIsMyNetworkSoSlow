FROM sophistsolutionsinc/stroika-buildvm-windows-msys-vs2k26:v3

SHELL ["cmd", "/S", "/C"]

# Node (26) and @quasar/cli come with the base image (Stroika's Windows-MSYS Dockerfile, INSTALL_QUASAR=1 - its default)

## hardwired location from sophistsolutionsinc/stroika-buildvm-windows-msys-vs2k26 dockerfile - find better way to compute!
RUN rm -f c:/msys64/home/Getting-Started-With-Stroika.md
ADD Shared-Files/Getting-Started.md c:/msys64/home/
RUN sed -i -e 's/Getting-Started-With-Stroika/Getting-Started/g' c:/msys64/home/.bash_profile

# CMD ["cmd", "/c", "call FixupPath.bat && echo see Getting-Started.md && cmd"]

### NOTE TO RUN
###     docker run -v c:\Sandbox:c:\Sandbox -it sophistsolutionsinc/whythefuckismynetworksoslow-windows-vs2k26
###     docker run -it sophistsolutionsinc/whythefuckismynetworksoslow-windows-vs2k26
