import { IDeviceFilterParams } from "@/models/device/query/IDeviceFilterParams";
import { IDeviceSortParams } from "@/models/device/query/IDeviceSortParams";

export interface IDeviceQuery {
    filterParams?: IDeviceFilterParams;
    sortParms?: IDeviceSortParams;
}
