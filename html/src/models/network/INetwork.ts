import { IGeographicLocation } from "../network/IGeographicLocation";
import { IInternetServiceProvider } from "../network/IInternetServiceProvider";
import { IDateTimeRange } from "../common/IDateTimeRange"

export interface INetwork {
  id: string;
  aggregatedBy?: string;
  aggregatesReversibly?: string[];
  aggregatesIrreversibly?: string[];
  idIsPersistent?: boolean;
  historicalSnapshot?: boolean;
  DNSServers: string[];
  attachedInterfaces: string[]; // MOSTLY IGNORED - but keep around cuz maybe handy in debugging
  externalAddresses: string[];
  gateways: string[];
  gatewayHardwareAddresses: string[];
  geographicLocation?: IGeographicLocation;
  internetServiceProvider: IInternetServiceProvider;
  networkAddresses: string[];
  names: { name: string, priority: number }[];
  seen?: IDateTimeRange;
  debugProps?: object;
}
