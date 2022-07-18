import { DeviceTypeEnum } from "@/models/device/DeviceTypeEnum";
import { OperatingSystem } from "@/models/OperatingSystem";
import { IDateTimeRange } from "../common/IDateTimeRange";

export interface INetworkAttachmentInfo {
  hardwareAddresses: string[];
  localAddresses: string[];
}

export interface IManufacturer {
  shortName?: string;
  fullName?: string;
  webSiteURL?: string;
}

export interface IDevice {
  id: string;
  aggregatesReversibly?: string[];
  aggregatesIrreversibly?: string[];
  idIsPersistent?: boolean;
  historicalSnapshot?: boolean;
  attachedNetworks: { [key: string]: INetworkAttachmentInfo };
  attachedNetworkInterfaces: string[];
  name: string;
  seen?: { [key: string]: IDateTimeRange };
  openPorts?: string[];
  icon?: string;
  manufacturer?: IManufacturer;
  type: Array<DeviceTypeEnum | string>;
  presentationURL?: string;
  operatingSystem?: OperatingSystem;
  debugProps?: object;
}
