FROM sophistsolutionsinc/stroika-buildvm-windows-cygwin-vs2k19

SHELL ["cmd", "/S", "/C"]

RUN choco install -y --force nodejs-lts
RUN npm i -g npm 

RUN rm -f /Getting-Started-With-Stroika.md
ADD Shared-Files/Getting-Started.md /

CMD ["cmd", "/c", "call FixupPath.bat && echo see Getting-Started.md && cmd"]

### NOTE TO RUN
###     docker run -v c:\Sandbox:c:\Sandbox -it sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19
###     docker run -it sophistsolutionsinc/whythefuckismynetworksoslow-windows-cygwin-vs2k19
