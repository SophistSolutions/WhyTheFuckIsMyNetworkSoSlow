import { createStore } from 'vuex'
import Vue from "vue";
// import Vuex from "vuex";

import { INetwork } from "@/models/network/INetwork";
import { INetworkInterface } from "@/models/network/INetworkInterface";

import { IDevice } from "@/models/device/IDevice";
import { ISortBy } from "@/models/device/SearchSpecification";
import { IAbout } from "@/models/IAbout";
import {
  fetchAboutInfo,
  fetchDevice,
  fetchDevices,
  fetchNetwork,
  fetchNetworkInterfaces,
  fetchNetworks
} from "@/proxy/API";

const debug = process.env.NODE_ENV !== "production";

// @TODO make this the root store - have other stores be props of state. Seperate by logical groups
export default createStore({
  state: {
    about: undefined as IAbout | undefined,
    rolledUpAvailableNetworkIDs: [] as string[],
    // cache of objects, some of which maybe primary networks (rollups) and some maybe details
    networkDetails: {} as { [key: string]: INetwork },
    networkInterfaces: [] as INetworkInterface[],
    selectedNetworkId: {} as string,
    rolledUpDeviceIDs: [] as string[],
    // cache of objects, some of which maybe primary devices (rollups) and some maybe details
    deviceDetails: {} as { [key: string]: IDevice }
  },
  mutations: {
    setAvailableNetworks(state, networks: INetwork[]) {
      state.rolledUpAvailableNetworkIDs = networks.map((x) => x.id);
      networks.forEach((x) => (state.networkDetails[x.id] = x));
      // networks.forEach((x) => Vue.set(state.networkDetails, x.id, x));
    },
    setNetworkDetails(state, network: INetwork) {
      state.networkDetails[network.id] = network;
      // Vue.set(state.networkDetails, network.id, network);
    },
    setNetworkInterfaces(state, networkInterfaces: INetworkInterface[]) {
      state.networkInterfaces = networkInterfaces;
    },
    setDevices(state, devices: IDevice[]) {
      state.rolledUpDeviceIDs = devices.map((x) => x.id);
      devices.forEach((x) => (state.deviceDetails[x.id] = x));
      // devices.forEach((x) => Vue.set(state.deviceDetails, x.id, x));
    },
    setDeviceDetails(state, device: IDevice) {
      state.deviceDetails[device.id] = device;
      // Vue.set(state.deviceDetails, device.id, device);
    },
    setSelectedNetwork(state, networkId: string) {
      state.selectedNetworkId = networkId;
    },
    setAboutInfo(state, aboutInfo: IAbout) {
      state.about = aboutInfo;
    }
  },
  actions: {
    async fetchAvailableNetworks({ commit }) {
      commit("setAvailableNetworks", await fetchNetworks());
    },
    async fetchNetworkInterfaces({ commit }) {
      commit("setNetworkInterfaces", await fetchNetworkInterfaces());
    },
    async fetchNetwork({ commit }, id: string) {
      commit("setNetworkDetails", await fetchNetwork(id));
    },
    async fetchNetworks({ commit }, ids: string[]) {
      ids.forEach(async (id) => commit("setNetworkDetails", await fetchNetwork(id)));
    },
    async fetchAboutInfo({ commit }) {
      commit("setAboutInfo", await fetchAboutInfo());
    },
    async fetchDevices({ commit }, searchSpecs: ISortBy) {
      commit("setDevices", await fetchDevices(searchSpecs));
    },
    async fetchDevice({ commit }, id: string) {
      commit("setDeviceDetails", await fetchDevice(id));
    },
    setSelectedNetwork({ commit }, networkId: string) {
      commit("setSelectedNetwork", networkId);
    }
  },
  modules: {
  },
  getters: {
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
    }
  },
  strict: debug
})
