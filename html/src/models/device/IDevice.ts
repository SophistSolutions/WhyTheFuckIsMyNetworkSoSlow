import { DeviceTypeEnum } from '@/models/device/DeviceTypeEnum'

export interface IDevice {
    id: string;
    name: string;
    internetAddresses: string[];
    type?: DeviceTypeEnum;
}
