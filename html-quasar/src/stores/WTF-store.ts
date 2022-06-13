import { defineStore } from 'pinia';

import { INetwork } from "../models/network/INetwork";
import { INetworkInterface } from "../models/network/INetworkInterface";

import { IDevice } from "../models/device/IDevice";
import { ISortBy } from "../models/device/SearchSpecification";
import { IAbout } from "../models/IAbout";
import {
  fetchAboutInfo,
  fetchDevice,
  fetchDevices,
  fetchNetwork,
  fetchNetworkInterfaces,
  fetchNetworks,
} from "../proxy/API";


const debug = process.env.NODE_ENV !== "production";


/// SAMPLE CODE
export const useCounterStore = defineStore('counter', {
  state: () => ({
    counter: 0,
  }),
  getters: {
    doubleCount: (state) => state.counter * 2,
  },
  actions: {
    increment() {
      this.counter++;
    },
  },
});


// @todo perhaps add in 'lasttimerequested' and 'lastTimeSuccessfulResponse' and throttle/dont request 
// (not sure where in model) if outtsanding requests etc) and maybe show in UI if data stale
interface ILoading {
  numberOfTimesLoaded: number;
  numberOfOutstandingLoadRequests: number;
}

/// DRAFT new WTF app data store  - maybe should be called cached-network-state-store?

export const useWTFStore = defineStore('WTF', {
  state: () => ({
    about: undefined as IAbout | undefined,
    rolledUpAvailableNetworkIDs: [] as string[],
    // cache of objects, some of which maybe primary networks (rollups) and some maybe details
    networkDetails: {} as { [key: string]: INetwork },
    loadingNetworks: { numberOfTimesLoaded: 0, numberOfOutstandingLoadRequests: 0 } as ILoading,
    networkInterfaces: [] as INetworkInterface[],
    selectedNetworkId: {} as string,
    rolledUpDeviceIDs: [] as string[],
    // cache of objects, some of which maybe primary devices (rollups) and some maybe details
    deviceDetails: {} as { [key: string]: IDevice },
    devicesLoading: { numberOfTimesLoaded: 0, numberOfOutstandingLoadRequests: 0 } as ILoading,
  }),
  getters: {
    getLoading_Networks: (state) => {
      return state.loadingNetworks;
    },
    getLoading_Devices: (state) => {
      return state.devicesLoading;
    },
    getAvailableNetworks: (state) => {
      return state.rolledUpAvailableNetworkIDs.map((di) => state.networkDetails[di]);
    },
    getNetwork: (state) => {
      return (id: string) => state.networkDetails[id];
    },
    getNetworkInterfaces: (state) => {
      return state.networkInterfaces;
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
    async fetchNetworkInterfaces() {
      this.networkInterfaces = await fetchNetworkInterfaces();
    },
    async fetchNetworks(ids: string[]) {
      this.loadingNetworks.numberOfOutstandingLoadRequests++;
      try {
        ids.forEach(async (id) => this.networkDetails[id] = await fetchNetwork(id));
        this.loadingNetworks.numberOfTimesLoaded++;
      }
      finally {
        this.loadingNetworks.numberOfOutstandingLoadRequests--;
      }
    },
    async fetchAboutInfo() {
      this.about = await fetchAboutInfo();
    },
    async fetchDevices(searchSpecs?: ISortBy) {
      this.devicesLoading.numberOfOutstandingLoadRequests++;
      try {
        const devices: IDevice[] = await fetchDevices(searchSpecs);
        this.rolledUpDeviceIDs = devices.map((x) => x.id);
        devices.forEach((x) => (this.deviceDetails[x.id] = x));
        this.devicesLoading.numberOfTimesLoaded++;
      }
      finally {
        this.devicesLoading.numberOfOutstandingLoadRequests--;
      }
    },
    async fetchDevice(id: string) {
      this.deviceDetails[id] = await fetchDevice(id);
    },
    setSelectedNetwork(networkId: string) {
      this.selectedNetworkId = networkId;
    },
  },
});

