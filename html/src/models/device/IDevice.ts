import { DeviceTypeEnum } from '@/models/device/DeviceTypeEnum';
import { OperatingSystem } from '@/models/OperatingSystem';
import { IDateTimeRange } from '../common/IDateTimeRange';

export interface INetworkAttachmentInfo {
  hardwareAddresses: string[];
  localAddresses: string[];
}

export interface IManufacturer {
  shortName?: string;
  fullName?: string;
  webSiteURL?: string;
}



export interface IDevice_UserOverridesType_NetworkInterfaceAggregateRule {
  interfaceType: string;
  fingerPrint: string;
}


export interface IDevice_UserOverridesType {
  name?: string;
  tags?: string[];
  notes?: string;
  aggregateNetworks?: string[];
  aggregateFingerprints?: string[];
  aggregateGatewayHardwareAddresses?: string[];
  aggregateNetworkInterfacesMatching?: IDevice_UserOverridesType_NetworkInterfaceAggregateRule[];
}

export interface IDevice {
  id: string;
  aggregatedBy?: string;
  aggregatesReversibly?: string[];
  aggregatesIrreversibly?: string[];
  idIsPersistent?: boolean;
  attachedNetworks: { [key: string]: INetworkAttachmentInfo };
  attachedNetworkInterfaces: string[];
  name: string;
  names: { name: string; priority: number }[];
  seen?: { [key: string]: IDateTimeRange };
  openPorts?: string[];
  icon?: URL;
  manufacturer?: IManufacturer;
  type: Array<DeviceTypeEnum | string>;
  presentationURL?: URL;
  operatingSystem?: OperatingSystem;
  userOverrides? :IDevice_UserOverridesType;
  debugProps?: object;
}
