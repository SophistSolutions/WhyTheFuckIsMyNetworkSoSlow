import { IGeographicLocation } from '@/models/Network/IGeographicLocation';
import { IInternetServiceProvider } from '@/models/Network/IInternetServiceProvider';

export interface INetwork {
    id: string;
    DNSServers: string[];
    attachedInterfaces: string[];
    externalAddresses: string[];
    gateways: string[];
    geographicLocation: IGeographicLocation;
    internetServiceProvider: IInternetServiceProvider;
    networkAddresses: string[];
}
