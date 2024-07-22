FROM sophistsolutionsinc/stroika-buildvm-windows-msys-vs2k22:v3

SHELL ["cmd", "/S", "/C"]

# RUN choco install -y --force nodejs-lts       # IF using cygwin instead of msys
RUN pacman --noconfirm -S mingw-w64-x86_64-nodejs
RUN npm i -g npm quasar

## hardwired location from sophistsolutionsinc/stroika-buildvm-windows-msys-vs2k22 dockerfile - find better way to compute!
RUN rm -f c:/msys64/home/Getting-Started-With-Stroika.md
ADD Shared-Files/Getting-Started.md c:/msys64/home/
RUN sed -i -e 's/Getting-Started-With-Stroika/Getting-Started/g' c:/msys64/home/.bash_profile

# CMD ["cmd", "/c", "call FixupPath.bat && echo see Getting-Started.md && cmd"]

### NOTE TO RUN
###     docker run -v c:\Sandbox:c:\Sandbox -it sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19
###     docker run -it sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19
