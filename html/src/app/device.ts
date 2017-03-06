
export class Device {
  name: string;
  ipv4: string;
  ipv6: string;
  ipAddresses: string[];
  type: string;
  network: string;
  networkMask: string;
  signalStrength: number;
  connected: boolean;
}
