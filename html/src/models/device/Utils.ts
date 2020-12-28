import { IDevice, INetworkAttachmentInfo } from "@/models/device/IDevice";
import { INetwork } from "@/models/network/INetwork";
import { OperatingSystem } from "@/models/OperatingSystem";

/*
 *  Return an array of image-url/label pairs, for the given operating system object
 */
export function ComputeOSIconURLList(
  t: OperatingSystem | null
): Array<{
  url?: string | null;
  label: string;
}> {
  const result: Array<{
    url?: string | null;
    label: string;
  }> = [];
  if (t) {
    if (t.fullVersionedName.startsWith("Windows")) {
      result.push({
        url: "images/WindowsOS.ico",
        label: t.fullVersionedName,
      });
    } else if (t.fullVersionedName.startsWith("Linux")) {
      result.push({
        url: "images/Linux.png",
        label: t.fullVersionedName,
      });
    } else if (t.fullVersionedName.startsWith("POSIX")) {
      result.push({
        url: "images/UnixOSIcon.png",
        label: t.fullVersionedName,
      });
    } else {
      result.push({
        label: t.fullVersionedName,
      });
    }
  }
  return result;
}

/*
 *  Return an array of image-url/label pairs, for the given array of device types.
 */
export function ComputeDeviceTypeIconURLs(
  t: string[] | null
): Array<{
  url?: string | null;
  label: string;
}> {
  const result: Array<{
    url?: string | null;
    label: string;
  }> = [];
  if (t) {
    t.forEach((ti: string) => {
      if (ti === "Router") {
        result.push({
          url: "images/RouterDevice.ico",
          label: ti,
        });
      } else if (ti === "Network-Infrastructure") {
        result.push({
          url: "images/network-infrastructure.ico",
          label: ti,
        });
      } else if (ti === "Personal-Computer") {
        result.push({
          url: "images/PC-Device.png",
          label: ti,
        });
      } else if (ti === "Speaker") {
        result.push({
          url: "images/SpeakerDeviceIcon.png",
          label: ti,
        });
      } else if (ti === "Printer") {
        result.push({
          url: "images/Printer.ico",
          label: ti,
        });
      } else if (ti === "TV") {
        result.push({
          url: "images/TV-Icon.png",
          label: ti,
        });
      } else if (ti === "Media-Player") {
        result.push({
          url: "images/Media-Player-Icon.png",
          label: ti,
        });
      } else {
        result.push({
          label: ti,
        });
      }
    });
  }
  return result;
}
