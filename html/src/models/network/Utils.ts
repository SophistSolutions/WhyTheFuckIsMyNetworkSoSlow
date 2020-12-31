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

export function FormatLocation(l: IGeographicLocation): string {
  let result: string = "";
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

export function FormatAttachedNetworkAddresses(attachedNetworks: {
  [key: string]: INetworkAttachmentInfo;
}): string {
  let addresses: string[] = [];
  Object.entries(attachedNetworks).forEach((element) => {
    element[1].networkAddresses.forEach((e: string) => addresses.push(e));
  });
  addresses = addresses.filter((value, index, self) => self.indexOf(value) === index);
  return addresses.join(", ");
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
