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
      switch (ti) {
        case "Network-Infrastructure":
          result.push({
            url: "images/network-infrastructure.ico",
            label: ti,
          });
          break;
        case "Media-Player":
          result.push({
            url: "images/Media-Player-Icon.png",
            label: ti,
          });
          break;
        case "Personal-Computer":
          result.push({
            url: "images/PC-Device.png",
            label: ti,
          });
          break;
        case "Printer":
          result.push({
            url: "images/Printer.ico",
            label: ti,
          });
          break;
        case "Router":
          result.push({
            url: "images/RouterDevice.ico",
            label: ti,
          });
          break;
        case "Speaker":
          result.push({
            url: "images/SpeakerDeviceIcon.png",
            label: ti,
          });
          break;
        case "TV":
          result.push({
            url: "images/TV-Icon.png",
            label: ti,
          });
          break;
        case "Virtual-Machine":
          result.push({
            url: "images/Virtual-Machine.png",
            label: ti,
          });
          break;
        default:
          result.push({
            label: ti,
          });
          break;
      }
    });
  }
  return result;
}

/*
 *  Returnimage-url/label for the given service type.
 */
export function ComputeServiceTypeIconURL(
  t: string
): {
  url?: string | null;
  label: string;
} {
  switch (t) {
    case "web":
      return {
        url: "images/web-page.png",
        label: t,
      };
    case "ssh":
      return {
        url: "images/ssh.png",
        label: t,
      };
    case "rdp":
      return {
        url: "images/rdp.png",
        label: t,
      };
    case "smb":
      return {
        url: "images/smb.png",
        label: t,
      };
    default:
      return {
        label: t,
      };
  }
}

export function ComputeServiceTypeIconURLs(
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
      result.push(ComputeServiceTypeIconURL(ti));
    });
  }
  return result;
}
