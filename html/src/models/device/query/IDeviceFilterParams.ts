import { DeviceTypeEnum } from "@/models/device/DeviceTypeEnum";

export interface IDeviceFilterParams {
    includeTypes?: DeviceTypeEnum[];
    excludeTypes?: DeviceTypeEnum[];
}
