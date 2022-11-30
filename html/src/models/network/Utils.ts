import { IDateTimeRange } from 'src/models/common/IDateTimeRange';
import moment from 'moment';

import { IDevice, INetworkAttachmentInfo } from '../../models/device/IDevice';
import { IGeographicLocation } from '../../models/network/IGeographicLocation';
import { INetwork } from '../../models/network/INetwork';


/**
 *  Format as 'an hour ago up until now' - and if summaryOnly true, then just return most recent time.
 * 
 * @param seenRange  
 * @param summaryOnly 
 * @returns undefined if bad or missing data in date range
 */
 export function FormatIDateTimeRange (
  seenRange?: IDateTimeRange,
  summaryOnly?: boolean
): string | undefined {
  if (seenRange) {
    const toText: string | undefined = seenRange?.upperBound? moment(seenRange.upperBound).fromNow(): undefined;
    if (summaryOnly) {
      return toText;
    }
    const fromText: string | undefined = seenRange?.lowerBound? moment(seenRange.lowerBound).fromNow(): undefined;
    if (fromText || toText) {
      return (fromText?? "") + ' up until ' +  (toText?? "");
    }
  }
  return undefined;
}
export function FormatSeenMap (
  seenRange?:  { [key: string]: IDateTimeRange },
  summaryOnly?: boolean
): string | undefined {
  if (seenRange && seenRange['ever']) {
    return FormatIDateTimeRange (seenRange['ever'], summaryOnly)
  }
  return undefined;
}

/*
 *  returns empty string if network address list is missing or empty
 *  \req n != null
 */
export function GetNetworkName(n: INetwork): string {
  // @todo this should probably sometimes be shortened and be more careful if no
  // freindlyname/networkaddresses, and maybe include reference to location (sudbury etc)
  let addresses = n.networkAddresses.filter((nn) => nn.includes('.'));
  if (addresses.length === 0) {
    addresses = n.networkAddresses;
  }
  const preferredName = n.names.length > 0 ? n.names[0].name : '';
  let name = preferredName;
  // allow user set names to be shown as is, but for lower priority names, fold the best name with the CIDR
  if (n.names.length == 0 || n.names[0].priority < 500) {
    name += ' {' + addresses.join(', ') + '}';
  }
  return name;
}

export function SortNetworks(nws: INetwork[]) {
  const result = Object.assign([], nws);
  result.sort((l: INetwork, r: INetwork) => {
    let res = -moment(l.seen?.upperBound).diff(r.seen?.upperBound);
    if (res == 0) {
      const lex = l.externalAddresses ? l.externalAddresses.length : 0;
      const rex = r.externalAddresses ? r.externalAddresses.length : 0;
      res = rex - lex;
    }
    return res;
  });
  return result;
}

export function GetNetworkCIDRs(n: INetwork): string {
  return n.networkAddresses.join(', ');
}

export function GetNetworkByID(
  networkID: string,
  networks: INetwork[]
): INetwork {
  const n: INetwork | undefined = GetNetworkByIDQuietly(networkID, networks);
  if (n === undefined) {
    throw new Error('no such network id found');
  }
  return n;
}

export function GetNetworkByIDQuietly(
  networkID: string,
  networks: INetwork[]
): INetwork | undefined {
  let n: INetwork;
  for (n of networks) {
    if (networkID === n.id) {
      return n;
    }
  }
  return undefined;
}

export function GetNetworkLink(n: INetwork | string): string | undefined {
  if (typeof n === 'string' || n instanceof String) {
    return `/#/network/${n}`;
  }
  if ((n as INetwork).id) {
    return GetNetworkLink((n as INetwork).id);
  }
  return undefined;
}

export function GetDevicesForNetworkLink(
  n: INetwork | string
): string | undefined {
  if (typeof n === 'string' || n instanceof String) {
    return `/#/devices?selectedNetwork=${n}`;
  }
  if ((n as INetwork).id) {
    return GetDevicesForNetworkLink((n as INetwork).id);
  }
  return undefined;
}

/**
 *
 * @param l argument can be null, in which case this returns null
 */
export function FormatLocation(l?: IGeographicLocation): string | null {
  let result: string = '';
  if (l == null) {
    return null;
  }
  if (l.city != null) {
    result += l.city;
  }
  if (l.regionCode != null) {
    if (result !== '') {
      result += ' ';
    }
    result += l.regionCode;
  }
  if (l.countryCode != null) {
    if (result !== '') {
      result += ', ';
    }
    result += l.countryCode;
  }
  if (l.postalCode != null) {
    if (result !== '') {
      result += ' ';
    }
    result += l.postalCode;
  }
  return result;
}

export function GetDeviceIDsInNetwork(
  nw: INetwork,
  devices: IDevice[]
): string[] {
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
  addresses = addresses.filter(
    (value, index, self) => self.indexOf(value) === index
  );
  return addresses.join(', ');
}

export function FormatAttachedNetwork(anw: INetworkAttachmentInfo): string {
  return anw.localAddresses.join(', ');
}

export function GetLocalNetworkAddresses(device: IDevice): string[] {
  const addresses: string[] = [];
  Object.entries(device.attachedNetworks).forEach((element) => {
    element[1].localAddresses.forEach((e: string) => addresses.push(e));
  });
  return addresses.filter(
    (value, index, self) => self.indexOf(value) === index
  );
}

/// And sort them the way we do throughout the rest of the UI
export function GetAttachedNetworksAsNetworks(
  attachedNetworks: {
    [key: string]: INetworkAttachmentInfo;
  },
  allNetworks: INetwork[]
): INetwork[] {
  const result = [] as INetwork[];
  Object.entries(attachedNetworks).forEach((element) => {
    const i = allNetworks.find((i) => i.id == element[0]);
    if (i) {
      result.push(i);
    }
  });
  return SortNetworks(result);
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
    if (device.openPorts.includes('tcp:515')) {
      localNetworkAddresses.forEach((la) =>
        links.push({ href: `lpd://${la}` })
      );
    }
    if (device.openPorts.includes('tcp:631')) {
      localNetworkAddresses.forEach((la) =>
        links.push({ href: `ipp://${la}` })
      );
    }
    if (links.length !== 0) {
      result.push({ name: 'print', links });
    }
  }
  if (device.openPorts && device.openPorts.includes('tcp:3389')) {
    const links: Array<{ href: string }> = [];
    localNetworkAddresses.forEach((la) => links.push({ href: `rdp://${la}` }));
    result.push({ name: 'rdp', links });
  }
  if (device.openPorts && device.openPorts.includes('tcp:5900')) {
    const links: Array<{ href: string }> = [];
    localNetworkAddresses.forEach((la) => links.push({ href: `vnc://${la}` }));
    result.push({ name: 'vnc', links });
  }
  if (device.openPorts && device.openPorts.includes('tcp:22')) {
    const links: Array<{ href: string }> = [];
    localNetworkAddresses.forEach((la) => links.push({ href: `ssh://@${la}` }));
    result.push({ name: 'ssh', links });
  }
  if (device.openPorts && device.openPorts.includes('tcp:139')) {
    const links: Array<{ href: string }> = [];
    localNetworkAddresses.forEach((la) => links.push({ href: `smb://${la}` }));
    result.push({ name: 'smb', links });
  }
  if (device.openPorts && device.openPorts.includes('tcp:23')) {
    const links: Array<{ href: string }> = [];
    localNetworkAddresses.forEach((la) =>
      links.push({ href: `telnet://@${la}` })
    );
    result.push({ name: 'telnet', links });
  }
  {
    const links: Array<{ href: string }> = [];
    if (device.presentationURL) {
      links.push({ href: device.presentationURL });
    }
    if (device.openPorts && device.openPorts.includes('tcp:80')) {
      localNetworkAddresses.forEach((la) =>
        links.push({ href: `http://${la}` })
      );
    }
    if (device.openPorts && device.openPorts.includes('tcp:443')) {
      localNetworkAddresses.forEach((la) =>
        links.push({ href: `https://${la}` })
      );
    }
    if (device.openPorts && device.openPorts.includes('tcp:8080')) {
      localNetworkAddresses.forEach((la) =>
        links.push({ href: `http://${la}:8080` })
      );
    }
    if (links.length !== 0) {
      result.push({ name: 'web', links });
    }
  }
  return result;
}
