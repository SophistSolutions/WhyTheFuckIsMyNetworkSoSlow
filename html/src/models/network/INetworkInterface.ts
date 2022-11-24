// INCOMPLETE list of fields, but a good start
export interface INetworkInterface {
  platformInterfaceID: string;
  id: string;
  friendlyName: string;
  description?: string;
  type: string;
  hardwareAddress?: string;
  transmitSpeedBaud?: number;
  receiveLinkSpeedBaud?: number;
  wirelessInformation?: object;
  boundAddressRanges?: string[];
  boundAddresses?: string[];
  gateways?: string[];
  DNSServers?: string[];
  status?: string[];
  aggregatesReversibly?: string[];
  aggregatesIrreversibly?: string[];
  idIsPersistent?: boolean;
  historicalSnapshot?: boolean;
  debugProps?: object;
}
