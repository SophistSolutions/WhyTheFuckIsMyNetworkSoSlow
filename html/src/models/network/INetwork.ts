import { IGeographicLocation } from "../network/IGeographicLocation";
import { IInternetServiceProvider } from "../network/IInternetServiceProvider";
import { IDateTimeRange } from "../common/IDateTimeRange"

export interface INetwork {
  id: string;
  aggregatesReversibly?: string[];
  aggregatesIrreversibly?: string[];
  idIsPersistent?: boolean;
  historicalSnapshot?: boolean;
  DNSServers: string[];
  attachedInterfaces: string[]; // MOSTLY IGNORED - but keep around cuz maybe handy in debugging
  externalAddresses: string[];
  gateways: string[];
  geographicLocation?: IGeographicLocation;
  internetServiceProvider: IInternetServiceProvider;
  networkAddresses: string[];
  friendlyName?: string;
  lastSeenAt?: Date;
  seen?: IDateTimeRange;
  debugProps?: object;
}
