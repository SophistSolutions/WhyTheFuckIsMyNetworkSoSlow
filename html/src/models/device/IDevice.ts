import { DeviceTypeEnum } from "@/models/device/DeviceTypeEnum";
import { OperatingSystem } from "@/models/OperatingSystem";

export interface IDevice {
    id: string;
    attachedNetworks: string[];
    name: string;
    internetAddresses: string[];
    type: Array<DeviceTypeEnum | string>;
    presentationURL?: string;
    operatingSystem?: OperatingSystem;
}
