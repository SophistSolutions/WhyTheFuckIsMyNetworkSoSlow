// INCOMPLETE list of fields, but a good start
export interface INetworkInterface {
  id: string;
  DNSServers?: string[];
  boundAddressRanges?: string[];
  boundAddresses?: string[];
  gateways?: string[];
  friendlyName?: string;
  hardwareAddress?: string;
  description?: string;
  transmitSpeedBaud?: number;
  receiveLinkSpeedBaud?: number;
  type?: string;
  wirelessInformation?: object;
  debugProps?: object;
}
