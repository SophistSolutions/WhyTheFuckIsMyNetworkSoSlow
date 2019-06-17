import { DeviceTypeEnum } from "@/models/device/DeviceTypeEnum";
import { OperatingSystem } from "@/models/OperatingSystem";

export interface INetworkAttachmentInfo {
    hardwareAddresses: string[];
    networkAddresses: string[];
}

export interface IDevice {
    id: string;
    attachedNetworks: { [key: string]: INetworkAttachmentInfo };
    name: string;
    icon?: string;
    type: Array<DeviceTypeEnum | string>;
    presentationURL?: string;
    operatingSystem?: OperatingSystem;
}
