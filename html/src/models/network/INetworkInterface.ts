export interface IWirelessInfo {
  SSID?: string;
  state?: string;
  connectionMode?: string;
  profileName?: string;
  BSSType?: string;
  MACAddress?: string;
  physicalConnectionType?: string;
  signalQuality?: number;
  securityEnabled?: boolean;
  x8021Enabled?: boolean;
  authAlgorithm?: string;
  cipher?: string;
}

export interface INetworkInterface {
  platformInterfaceID: string;
  id: string;
  friendlyName: string;
  description?: string;
  type: string;
  hardwareAddress?: string;
  transmitSpeedBaud?: number;
  receiveLinkSpeedBaud?: number;
  wirelessInformation?: IWirelessInfo;
  boundAddressRanges?: string[];
  boundAddresses?: string[];
  gateways?: string[];
  DNSServers?: string[];
  status?: string[];
  aggregatedBy?: string;
  aggregatesReversibly?: string[];
  aggregatesIrreversibly?: string[];
  idIsPersistent?: boolean;
  debugProps?: object;
}

