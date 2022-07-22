import { IGeographicLocation } from "@/models/network/IGeographicLocation";
import { IInternetServiceProvider } from "@/models/network/IInternetServiceProvider";

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
}