# Device Discovery Notes

Various hints about a handful of devices we want to improve discovery on

- ComfortLink 1050 (I THINK)

  - two devices with mfg = Murata Manufacturing

    are probably my heat control devices

  - did port scan with nmap

        telnet 192.168.244.152 37447

        produces Your friendly neighborhood debug shell.
          (sometimes on port 45999)

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

- Apple Devices

  tps://www.speedguide.net/port.php?port=12080 phone-video used on macbook?

  MAYBE add links to
  https://www.speedguide.net/port.php?port=NNN
  for 'device open-ports' section

  https://www.speedguide.net/port.php?port=49152 (many things use this port)

      MACBOOK PRO @ 192.168.244.65 (nmap guessed appleTV 5)
      ports
        3445  (never responds when you send garbage)
        4502  (2 newlines causes hangup/end)
        12080 (hangup after 1 bad cahr)
        12110
        12143
        12443
        12993
        12995
        49152
        49279
        55695

      APPLE DEVICE @ 192.168.244.92 (Lewis-Mac)  - nmap guessed apple ios 11
      ports
        22
        88 kerberos-sec
        5900  (apple remote desktop vnc)

      APPLE DEVICE @ 192.168.244.119 (MACBOOKPRO-7E10) - nmap guessed apple ipod touch or old iphone
      ports
        52972

      APPLE DEVICE @ 192.168.244.183
      ports
        22
        5900 VNC (@todo add to my regular common scans)

  netcat 192.168.244.92 22
  prints some stuff out I can use to analye (but maybe not enuf)
