import { defineStore } from 'pinia';

import { INetwork } from '../models/network/INetwork';
import { INetworkInterface } from '../models/network/INetworkInterface';

import { IDevice } from '../models/device/IDevice';
import { ISortBy } from '../models/device/SearchSpecification';
import { IAbout } from '../models/IAbout';
import { Equals } from '../utils/Objects';
import {
  fetchAboutInfo,
  fetchDevice,
  fetchDevices,
  fetchNetwork,
  fetchNetworks,
  fetchNetworkInterface,
} from '../proxy/API';

import { kCompileTimeConfiguration } from '../config/config';


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
    rolledUpAvailableNetworkIDs: new Set() as Set<string>,
    // cache of objects, some of which maybe primary networks (rollups) and some maybe details
    networkDetails: {} as { [key: string]: INetwork },
    loadingActiveNetworks: {
      numberOfTimesLoaded: 0,
      numberOfOutstandingLoadRequests: 0,
    } as ILoading,
    loadingNetworkDetailsForID: new Set() as Set<string>,
    networkInterfaces: {} as { [key: string]: INetworkInterface },
    loadingNetworkInterfaceDetailsForID: new Set() as Set<string>,
    rolledUpDeviceIDs: new Set() as Set<string>,
    // cache of objects, some of which maybe primary devices (rollups) and some maybe details
    deviceDetails: {} as { [key: string]: IDevice },
    loadingActiveDevices: {
      numberOfTimesLoaded: 0,
      numberOfOutstandingLoadRequests: 0,
    } as ILoading,
    loadingDeviceDetailsForID: new Set() as Set<string>,
  }),
  getters: {
    getLoading_Networks: (state) => {
      return state.loadingActiveNetworks;
    },
    getLoading_Devices: (state) => {
      return state.loadingActiveDevices;
    },
    getAvailableNetworks: (state) => {
      return [...state.rolledUpAvailableNetworkIDs].flatMap((ni) => state.networkDetails[ni]?[state.networkDetails[ni]]:[] );
    },
    getNetwork: (state) => {
      return (id: string) => state.networkDetails[id];
    },
    getNetworkInterface: (state) => {
      return (id: string): INetworkInterface | undefined => state.networkInterfaces[id];
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
      return [...state.rolledUpDeviceIDs].flatMap((di) => state.deviceDetails[di]?[state.deviceDetails[di]]:[]);
    },
    getDevice: (state) => {
      return (id: string) => state.deviceDetails[id];
    },
    getAboutInfo: (state) => {
      return state.about;
    },
  },
  actions: {
    async fetchAvailableNetworks() {
      if (this.loadingActiveNetworks.numberOfOutstandingLoadRequests > 2) {
        console.log('Suppressing fetchAvailableNetworks due to WSAPI delays');
        return;
      }
      this.loadingActiveNetworks.numberOfOutstandingLoadRequests++;
      let networkIDs = [];
      try {
        networkIDs = await fetchNetworks();
        // for now, fetch network before adding to rolledUpAvailableNetworkIDs, but really should
        // do other way and make calling code more OK with when details still null
        // if you reorder with teh code as is (--LGP 2022-12-13) - get sporadic failures running hitting nulls
        // as stuff loads
        this.rolledUpAvailableNetworkIDs.clear();//UNSURE but i think best
        networkIDs.forEach(async (i: string) => this.rolledUpAvailableNetworkIDs.add(i))
        this.loadingActiveNetworks.numberOfTimesLoaded++;
      }
      finally {
        this.loadingActiveNetworks.numberOfOutstandingLoadRequests--;
      }
      //  %todo reconsider - no need to fetch these just cuz we fetched network ids - many need if NEW ones (maybe track those) - but otherwise let soemting
      // exlse express interest in that id, and fetch it --LGP 2023-03-02
      networkIDs.forEach(async (i: string) => await this.fetchNetwork(i));
    },
    async fetchNetworkInterfaces(ids: string[]) {
      ids.forEach(async (i: string) => {
        if (this.loadingNetworkInterfaceDetailsForID.has(i)) {
          console.log(
            `Suppressing fetchNetworkInterface ${i} since in progress (throttling)`
          );
        } else {
          this.loadingNetworkInterfaceDetailsForID.add(i);
          try {
            this.networkInterfaces[i] = await fetchNetworkInterface(i);
          } finally {
            this.loadingNetworkInterfaceDetailsForID.delete(i);
          }
        }
      });
    },
    async fetchNetworks(ids: string[]) {
      ids.forEach(async (id) => await this.fetchNetwork(id));
    },
    async fetchNetwork(i: string) {
      if (this.loadingNetworkDetailsForID.has(i)) {
        console.log(
          `Suppressing fetchNetwork ${i} since in progress (throttling)`
        );
      } else {
        this.loadingNetworkDetailsForID.add(i);
        try {
          // Suprising and disapointing that Equals optimization helps so much
          // I would have expected Pinia to do this check
          const r = await fetchNetwork(i);
          if (!Equals(r, this.networkDetails[i])) {
            if (kCompileTimeConfiguration.DEBUG_MODE) {
              console.log("new network value: id=", r.id)
            }
            this.networkDetails[i] = r;
          }
        } finally {
          this.loadingNetworkDetailsForID.delete(i);
        }
      }
    },
    async fetchAboutInfo() {
      this.about = await fetchAboutInfo();
    },
    async fetchActiveDevices(searchSpecs?: ISortBy) {
      // primitive WSAPI throttling
      if (this.loadingActiveDevices.numberOfOutstandingLoadRequests > 2) {
        console.log('Suppressing fetchActiveDevices due to WSAPI delays');
        return;
      }
      this.loadingActiveDevices.numberOfOutstandingLoadRequests++;
      let devices: string[] = [];
      try {
        devices = await fetchDevices(searchSpecs);
        this.rolledUpDeviceIDs.clear(); // unclear but probably sensible - careful of reactivity
        devices.forEach(async (i: string) => this.rolledUpDeviceIDs.add(i))
        this.loadingActiveDevices.numberOfTimesLoaded++;
      } finally {
        this.loadingActiveDevices.numberOfOutstandingLoadRequests--;
      }
      //  %todo reconsider - no need to fetch these just cuz we fetched devices ids - many need if NEW ones (maybe track those) - but otherwise let soemting
      // exlse express interest in that id, and fetch it --LGP 2023-03-02
      this.fetchDevices(devices); // after reset numberOutstanding...
    },
    async fetchDevice(i: string) {
      if (this.loadingDeviceDetailsForID.has(i)) {
        console.log(
          `Suppressing fetchDevice ${i} since in progress (throttling)`
        );
      } else {
        this.loadingDeviceDetailsForID.add(i);
        try {
          // Suprising and disapointing that Equals optimization helps so much
          // I would have expected Pinia to do this check
          const r = await fetchDevice(i);
          if (!Equals(r, this.deviceDetails[i])) {
            if (kCompileTimeConfiguration.DEBUG_MODE) {
              console.log("new device value: named=", r.name)
            }
            this.deviceDetails[i] = r;
          }
        } finally {
          this.loadingDeviceDetailsForID.delete(i);
        }
      }
    },
    async fetchDevices(ids: Array<string>) {
      ids.forEach(async (i) => (this.fetchDevice(i)));
    },
  },
});
