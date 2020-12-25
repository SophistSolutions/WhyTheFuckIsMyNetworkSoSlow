# Device Discovery Notes

Various hints about a handful of devices we want to improve discovery on

- ComfortLink 1050 (I THINK)

  - two devices with mfg = Murata Manufacturing

    are probably my heat control devices

  - did port scan with nmap

        telnet 192.168.244.152 37447

        produces Your friendly neighborhood debug shell.

  - TCPOPEN:
    37447
    49013
    52541
    57659

  - second device 192.168.244.172 -
    46869/tcp open unknown
    53399/tcp open unknown
    56671/tcp open unknown
    60024/tcp open unknown

  - MAYBE trick to dientifying is that resolved hostname starts with
    XL857- and then a string of hex digits (which didnt turn out to be mac address)

  - telnet 192.168.244.172 60024
    gets same freindly neighborhood shell
