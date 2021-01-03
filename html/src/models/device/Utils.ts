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
 *  Return an array of image-url/label pairs, for the given array of device types.
 */
export function ComputeServiceTypeIconURL(
  t: string
): {
  url?: string | null;
  label: string;
} {
  const result: Array<{
    url?: string | null;
    label: string;
  }> = [];
  if (t) {
    t.forEach((ti: string) => {
      switch (ti) {
        case "web":
          result.push({
            url: "images/web-page.png",
            label: ti,
          });
          break;
        case "ssh":
          result.push({
            url: "images/ssh.png",
            label: ti,
          });
          break;
        case "rdp":
          result.push({
            url: "images/rdp.png",
            label: ti,
          });
          break;
        case "smb":
          result.push({
            url: "images/smb.png",
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
      switch (ti) {
        case "web":
          result.push({
            url: "images/web-page.png",
            label: ti,
          });
          break;
        case "ssh":
          result.push({
            url: "images/ssh.png",
            label: ti,
          });
          break;
        case "rdp":
          result.push({
            url: "images/rdp.png",
            label: ti,
          });
          break;
        case "smb":
          result.push({
            url: "images/smb.png",
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
