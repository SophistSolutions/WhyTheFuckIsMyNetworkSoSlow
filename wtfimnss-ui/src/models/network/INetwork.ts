import { IGeographicLocation } from '@/models/network/IGeographicLocation';
import { IInternetServiceProvider } from '@/models/network/IInternetServiceProvider';

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
