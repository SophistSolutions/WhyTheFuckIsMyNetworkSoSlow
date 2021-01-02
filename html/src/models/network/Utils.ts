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
    return `/#/network/${(n as INetwork).id}`;
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
    element[1].networkAddresses.forEach((e: string) => addresses.push(e));
  });
  addresses = addresses.filter((value, index, self) => self.indexOf(value) === index);
  return addresses.join(", ");
}

export function FormatAttachedNetwork(anw: INetworkAttachmentInfo): string {
  return anw.networkAddresses.join(", ");
}

/*
 */
export function FormatAttachedNetworks(
  attachedNetworks: { [key: string]: INetworkAttachmentInfo },
  networks: INetwork[]
): string {
  let addresses: string[] = [];
  Object.entries(attachedNetworks).forEach((element) => {
    let netID = element[0];
    networks.forEach((network: INetwork) => {
      if (network.id === netID) {
        if (network.networkAddresses.length >= 1) {
          netID = network.networkAddresses[0];
        }
      }
    });
    addresses.push(netID);
  });
  addresses = addresses.filter((value, index, self) => self.indexOf(value) === index);
  return addresses.join(", ");
}
