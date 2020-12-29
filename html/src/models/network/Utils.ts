import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import { INetwork } from "@/models/network/INetwork";
import { OperatingSystem } from "@/models/OperatingSystem";

/*
 */
export function GetNetworkName(n: INetwork): String {
  return n.networkAddresses.join(", ");
}
