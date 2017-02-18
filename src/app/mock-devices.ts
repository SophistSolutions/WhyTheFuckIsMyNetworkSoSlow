import { Device }     from './device';

export const DEVICES: Device[] = [

	{
		  name: "Robert's Phone",
		  ipv4: "192.168.244.34",
		  ipv6: "fe80::ec4:7aff:fec7:7f1c",
		  type: "Phone",
		  image: "../images/phone.png",
		  network: "192.168.244.0/24",
  		  networkMask: "255.255.255.0",
		  signalStrength: 67,
		  connected: true
	},
	{
		  name: "WAP - Main",
		  ipv4: "192.168.244.87",
		  ipv6: "fe80::ea3:5fef:fed7:98cc",
		  type: "WAP",
		  image: "../images/WAP.png",
		  network: "192.168.244.0/24",
  		  networkMask: "255.255.255.0",
		  signalStrength: 34,
		  connected: true
	},
	{
		  name: "Robert's Mac",
		  ipv4: "192.168.244.114",
		  ipv6: "fe80::cc4:2aef:fec7:901c",
		  type: "Laptop",
		  image: "../images/laptop2.png",
		  network: "192.168.244.0/24",
  		  networkMask: "255.255.255.0",
		  signalStrength: 0,
		  connected: false
	},
	{
		  name: "House Router",
		  ipv4: "192.168.244.1",
		  ipv6: "fe80::ec4:3a4f:fe00:2f1f",
		  type: "Router",
		  image: "../images/router.png",
		  network: "192.168.244.0/24",
  		  networkMask: "255.255.255.0",
		  signalStrength: 36,
		  connected: true
	},
	{
		  name: "Chromecast",
		  ipv4: "192.168.244.11",
		  ipv6: "fe80::ec4:f7af:ee71:5ec1",
		  type: "Chromecast",
		  image: "../images/chromecast.png",
		  network: "192.168.244.0/24",
  		  networkMask: "255.255.255.0",
		  signalStrength: 137,
		  connected: true
	},
	{
		  name: "Printer",
		  ipv4: "192.168.244.121",
		  ipv6: "fe80::ec4:7aef:2c47:93e0",
		  type: "Printer",
		  image: "../images/printer.png",
		  network: "192.168.244.0/24",
  		  networkMask: "255.255.255.0",
		  signalStrength: 42,
		  connected: true
	},
	{
		  name: "Lewis' Laptop",
		  ipv4: "192.168.244.89",
		  ipv6: "fe80::ec4:3a4f:fe00:2f1f",
		  type: "Laptop",
		  image: "../images/laptop2.png",
		  network: "192.168.244.0/24",
  		  networkMask: "255.255.255.0",
		  signalStrength: 42,
		  connected: true
	},
	{
		  name: "Roku",
		  ipv4: "192.168.244.71",
		  ipv6: "fe80::ec4:f7af:ee71:5ec1",
		  type: "Roku",
		  image: "../images/chromecast.png",
		  network: "192.168.244.0/24",
  		  networkMask: "255.255.255.0",
		  signalStrength: 123,
		  connected: true
	},
	{
		  name: "Tablet",
		  ipv4: "192.168.244.101",
		  ipv6: "fe80::ec4:7aef:2c47:93e0",
		  type: "Tablet",
		  image: "../images/laptop2.png",
		  network: "192.168.244.0/24",
  		  networkMask: "255.255.255.0",
		  signalStrength: 12,
		  connected: true
	},


]