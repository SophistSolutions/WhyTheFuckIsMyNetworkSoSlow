import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import { IGeographicLocation } from "@/models/network/IGeographicLocation";
import { INetwork } from "@/models/network/INetwork";
import { OperatingSystem } from "@/models/OperatingSystem";

/*
 *  returns empty string if network address list is missing or empty
 *  \req n != null
 */
export function GetNetworkName(n: INetwork): string {
  return n.networkAddresses.join(", ");
}

export function GetNetworkByID(networkID: string, networks: INetwork[]): INetwork {
  const n: INetwork | null = GetNetworkByIDQuietly(networkID, networks);
  if (n == null) {
    throw new Error("no such network id found");
  }
  return n;
}

export function GetNetworkByIDQuietly(networkID: string, networks: INetwork[]): INetwork | null {
  let n: INetwork;
  for (n of networks) {
    if (networkID === n.id) {
      return n;
    }
  }
  return null;
}

export function GetNetworkLink(n: INetwork | string): string | null {
  if (typeof n === "string" || n instanceof String) {
    return `/#/network/${n}`;
  }
  if ((n as INetwork).id) {
    return GetNetworkLink((n as INetwork).id);
  }
  return null;
}

export function GetDevicesForNetworkLink(n: INetwork | string): string | null {
  if (typeof n === "string" || n instanceof String) {
    return `/#/devices?selectedNetwork=${n}`;
  }
  if ((n as INetwork).id) {
    return GetDevicesForNetworkLink((n as INetwork).id);
  }
  return null;
}

/**
 *
 * @param l argument can be null, in which case this returns ""
 */
export function FormatLocation(l?: IGeographicLocation): string {
  let result: string = "";
  if (l == null) {
    return result;
  }
  if (l.city != null) {
    result += l.city;
  }
  if (l.regionCode != null) {
    if (result !== "") {
      result += " ";
    }
    result += l.regionCode;
  }
  if (l.countryCode != null) {
    if (result !== "") {
      result += ", ";
    }
    result += l.countryCode;
  }
  if (l.postalCode != null) {
    if (result !== "") {
      result += " ";
    }
    result += l.postalCode;
  }
  return result;
}

export function GetDeviceIDsInNetwork(nw: INetwork, devices: IDevice[]): string[] {
  const ids: string[] = [];
  devices.forEach((i: IDevice) => {
    let hasThisNetwork = false;
    Object.entries(i.attachedNetworks).forEach((element) => {
      if (element[0] === nw.id) {
        hasThisNetwork = true;
      }
    });
    if (hasThisNetwork) {
      ids.push(i.id);
    }
  });
  return ids;
}

/*
 *
 */
export function FormatAttachedNetworkLocalAddresses(attachedNetworks: {
  [key: string]: INetworkAttachmentInfo;
}): string {
  let addresses: string[] = [];
  Object.entries(attachedNetworks).forEach((element) => {
    element[1].localAddresses.forEach((e: string) => addresses.push(e));
  });
  addresses = addresses.filter((value, index, self) => self.indexOf(value) === index);
  return addresses.join(", ");
}

export function FormatAttachedNetwork(anw: INetworkAttachmentInfo): string {
  return anw.localAddresses.join(", ");
}

export function GetLocalNetworkAddresses(device: IDevice): string[] {
  const addresses: string[] = [];
  Object.entries(device.attachedNetworks).forEach((element) => {
    element[1].localAddresses.forEach((e: string) => addresses.push(e));
  });
  return addresses.filter((value, index, self) => self.indexOf(value) === index);
}

/**
 * returned devices are { name: .e.g ssh, links: [{href: telnet://202.2.2.2}] }, returns array of them
 * @param device
 */
export function GetServices(
  device: IDevice
): Array<{ name: string; links: Array<{ href: string }> }> {
  const localNetworkAddresses = GetLocalNetworkAddresses(device);
  const result: Array<{ name: string; links: Array<{ href: string }> }> = [];
  // see https://www.w3.org/wiki/UriSchemes
  // @todo add more, but this is probably the most important ones to list...
  if (device.openPorts) {
    const links: Array<{ href: string }> = [];
    if (device.openPorts.includes("tcp:515")) {
      localNetworkAddresses.forEach((la) => links.push({ href: `lpd://${la}` }));
    }
    if (device.openPorts.includes("tcp:631")) {
      localNetworkAddresses.forEach((la) => links.push({ href: `ipp://${la}` }));
    }
    if (links.length !== 0) {
      result.push({ name: "print", links });
    }
  }
  if (device.openPorts && device.openPorts.includes("tcp:3389")) {
    const links: Array<{ href: string }> = [];
    localNetworkAddresses.forEach((la) => links.push({ href: `rdp://${la}` }));
    result.push({ name: "rdp", links });
  }
  if (device.openPorts && device.openPorts.includes("tcp:5900")) {
    const links: Array<{ href: string }> = [];
    localNetworkAddresses.forEach((la) => links.push({ href: `vnc://${la}` }));
    result.push({ name: "vnc", links });
  }
  if (device.openPorts && device.openPorts.includes("tcp:22")) {
    const links: Array<{ href: string }> = [];
    localNetworkAddresses.forEach((la) => links.push({ href: `ssh://@${la}` }));
    result.push({ name: "ssh", links });
  }
  if (device.openPorts && device.openPorts.includes("tcp:139")) {
    const links: Array<{ href: string }> = [];
    localNetworkAddresses.forEach((la) => links.push({ href: `smb://${la}` }));
    result.push({ name: "smb", links });
  }
  if (device.openPorts && device.openPorts.includes("tcp:23")) {
    const links: Array<{ href: string }> = [];
    localNetworkAddresses.forEach((la) => links.push({ href: `telnet://@${la}` }));
    result.push({ name: "telnet", links });
  }
  {
    const links: Array<{ href: string }> = [];
    if (device.presentationURL) {
      links.push({ href: device.presentationURL });
    }
    if (device.openPorts && device.openPorts.includes("tcp:80")) {
      localNetworkAddresses.forEach((la) => links.push({ href: `http://${la}` }));
    }
    if (device.openPorts && device.openPorts.includes("tcp:443")) {
      localNetworkAddresses.forEach((la) => links.push({ href: `https://${la}` }));
    }
    if (device.openPorts && device.openPorts.includes("tcp:8080")) {
      localNetworkAddresses.forEach((la) => links.push({ href: `http://${la}:8080` }));
    }
    if (links.length !== 0) {
      result.push({ name: "web", links });
    }
  }
  return result;
}
