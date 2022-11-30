import { defineStore } from 'pinia';

import { INetwork } from '../models/network/INetwork';
import { INetworkInterface } from '../models/network/INetworkInterface';

import { IDevice } from '../models/device/IDevice';
import { ISortBy } from '../models/device/SearchSpecification';
import { IAbout } from '../models/IAbout';
import {
  fetchAboutInfo,
  fetchDevice,
  fetchDevices,
  fetchNetwork,
  fetchNetworkInterface,
  fetchNetworks,
} from '../proxy/API';

const debug = process.env.NODE_ENV !== 'production';

// @todo perhaps add in 'lasttimerequested' and 'lastTimeSuccessfulResponse' and throttle/dont request
// (not sure where in model) if outtsanding requests etc) and maybe show in UI if data stale
interface ILoading {
  numberOfTimesLoaded: number;
  numberOfOutstandingLoadRequests: number;
}

/// DRAFT new WTF app data store  - maybe should be called cached-network-state-store?

export const useNetStateStore = defineStore('Net-State-Store', {
  state: () => ({
    about: undefined as IAbout | undefined,
    rolledUpAvailableNetworkIDs: [] as string[],
    // cache of objects, some of which maybe primary networks (rollups) and some maybe details
    networkDetails: {} as { [key: string]: INetwork },
    loadingNetworks: {
      numberOfTimesLoaded: 0,
      numberOfOutstandingLoadRequests: 0,
    } as ILoading,
    networkInterfaces: {} as { [key: string]: INetworkInterface },
    selectedNetworkId: {} as string,
    rolledUpDeviceIDs: [] as string[],
    // cache of objects, some of which maybe primary devices (rollups) and some maybe details
    deviceDetails: {} as { [key: string]: IDevice },
    devicesLoading: {
      numberOfTimesLoaded: 0,
      numberOfOutstandingLoadRequests: 0,
    } as ILoading,
    // @todo - incomplete/draft attempt to track what we are already loading
    networkInterfacesLoading: new Set() as Set<string>,
  }),
  getters: {
    getLoading_Networks: (state) => {
      return state.loadingNetworks;
    },
    getLoading_Devices: (state) => {
      return state.devicesLoading;
    },
    getAvailableNetworks: (state) => {
      return state.rolledUpAvailableNetworkIDs.map(
        (di) => state.networkDetails[di]
      );
    },
    getNetwork: (state) => {
      return (id: string) => state.networkDetails[id];
    },
    getNetworkInterface: (state) => {
      return (id: string): INetworkInterface => state.networkInterfaces[id];
    },
    getNetworkInterfaces: (state) => {
      // untested as of 2022-11-24 - not sure the reduce part is right
      return (ids: string[]) =>
        Object.keys(state.networkInterfaces)
          .filter((key) => ids.includes(key))
          .reduce((obj: { [key: string]: INetworkInterface }, key) => {
            obj[key] = state.networkInterfaces[key];
            return obj;
          }, {});
    },
    getDevices: (state) => {
      return state.rolledUpDeviceIDs.map((di) => state.deviceDetails[di]);
    },
    getDevice: (state) => {
      return (id: string) => state.deviceDetails[id];
    },
    getSelectedNetworkId: (state) => {
      return state.selectedNetworkId;
    },
    getAboutInfo: (state) => {
      return state.about;
    },
    getSelectedNetworkObject: (state) => {
      return state.rolledUpAvailableNetworkIDs.find(
        (network) => network === state.selectedNetworkId
      );
    },
  },
  actions: {
    async fetchAvailableNetworks() {
      const networks: INetwork[] = await fetchNetworks();
      this.rolledUpAvailableNetworkIDs = networks.map((x) => x.id);
      networks.forEach((x) => (this.networkDetails[x.id] = x));
    },
    async fetchNetworkInterfaces(ids: string[]) {
      ids.forEach(async (i: string) => {
        if (this.networkInterfacesLoading.has(i)) {
          console.log(
            'Suppressing fetchNetworkInterface ${i} since in progress (throttling)'
          );
        } else {
          this.networkInterfacesLoading.add(i);
          try {
            this.networkInterfaces[i] = await fetchNetworkInterface(i);
          } finally {
            this.networkInterfacesLoading.delete(i);
          }
        }
      });
    },
    async fetchNetworks(ids: string[]) {
      // primitive WSAPI throttling
      if (this.devicesLoading.numberOfOutstandingLoadRequests > 2) {
        console.log('Suppressing fetchNetworks due to WSAPI delays');
        return;
      }
      this.loadingNetworks.numberOfOutstandingLoadRequests++;
      try {
        ids.forEach(
          async (id) => (this.networkDetails[id] = await fetchNetwork(id))
        );
        this.loadingNetworks.numberOfTimesLoaded++;
      } finally {
        this.loadingNetworks.numberOfOutstandingLoadRequests--;
      }
    },
    async fetchAboutInfo() {
      this.about = await fetchAboutInfo();
    },
    async fetchActiveDevices(searchSpecs?: ISortBy) {
      // primitive WSAPI throttling
      if (this.devicesLoading.numberOfOutstandingLoadRequests > 2) {
        console.log('Suppressing fetchActiveDevices due to WSAPI delays');
        return;
      }
      this.devicesLoading.numberOfOutstandingLoadRequests++;
      try {
        const devices: IDevice[] = await fetchDevices(searchSpecs);
        this.rolledUpDeviceIDs = devices.map((x) => x.id);
        devices.forEach((x) => (this.deviceDetails[x.id] = x));
        this.devicesLoading.numberOfTimesLoaded++;
      } finally {
        this.devicesLoading.numberOfOutstandingLoadRequests--;
      }
    },
    async fetchDevice(id: string) {
      this.deviceDetails[id] = await fetchDevice(id);
    },
    async fetchDevices(ids: Array<string>) {
      ids.forEach(async (i) => (this.deviceDetails[i] = await fetchDevice(i)));
    },
    setSelectedNetwork(networkId: string) {
      this.selectedNetworkId = networkId;
    },
  },
});
