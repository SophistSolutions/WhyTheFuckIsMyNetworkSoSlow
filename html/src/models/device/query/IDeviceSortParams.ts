import { DeviceSortByFieldEnum } from "@/models/device/query/DeviceSortByFieldEnum";
import { SortOrderEnum } from "@/models/SortOrderEnum";

export interface IDeviceSortParams {
    sortBy: DeviceSortByFieldEnum[]; // Array of fields to sort by
    sortOrder?: SortOrderEnum;
}
