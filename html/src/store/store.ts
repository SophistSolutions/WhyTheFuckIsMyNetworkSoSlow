import Vue from "vue";
import Vuex from "vuex";

import { INetwork } from "@/models/network/INetwork";
import { INetworkInterface } from "@/models/network/INetworkInterface";

import { IDevice } from "@/models/device/IDevice";
import { ISortBy } from "@/models/device/SearchSpecification";
import { IAbout } from "@/models/IAbout";
import { fetchAboutInfo, fetchDevices, fetchNetworkInterfaces, fetchNetworks } from "@/proxy/API";

Vue.use(Vuex);

const debug = process.env.NODE_ENV !== "production";

// TODO make this the root store - have other stores be props of state. Seperate by logical groups
export default new Vuex.Store({
  state: {
    about: undefined as IAbout | undefined,
    availableNetworks: [] as INetwork[],
    networkInterfaces: [] as INetworkInterface[],
    selectedNetworkId: {} as string,
    devices: [] as IDevice[],
  },
  mutations: {
    setAvailableNetworks(state, networks: INetwork[]) {
      state.availableNetworks = networks;
    },
    setNetworkInterfaces(state, networkInterfaces: INetworkInterface[]) {
      state.networkInterfaces = networkInterfaces;
    },
    setDevices(state, devices: IDevice[]) {
      state.devices = devices;
    },
    setSelectedNetwork(state, networkId: string) {
      state.selectedNetworkId = networkId;
    },
    setAboutInfo(state, aboutInfo: IAbout) {
      state.about = aboutInfo;
    },
  },
  actions: {
    async fetchAvailableNetworks({ commit }) {
      commit("setAvailableNetworks", await fetchNetworks());
    },
    async fetchNetworkInterfaces({ commit }) {
      commit("setNetworkInterfaces", await fetchNetworkInterfaces());
    },
    async fetchAboutInfo({ commit }) {
      commit("setAboutInfo", await fetchAboutInfo());
    },
    async fetchDevices({ commit }, searchSpecs: ISortBy) {
      commit("setDevices", await fetchDevices(searchSpecs));
    },
    setSelectedNetwork({ commit }, networkId: string) {
      commit("setSelectedNetwork", networkId);
    },
  },
  getters: {
    getAvailableNetworks: (state) => {
      return state.availableNetworks;
    },
    getNetworkInterfaces: (state) => {
      return state.networkInterfaces;
    },
    getDevices: (state) => {
      return state.devices;
    },
    getSelectedNetworkId: (state) => {
      return state.selectedNetworkId;
    },
    getAboutInfo: (state) => {
      return state.about;
    },
    getSelectedNetworkObject: (state) => {
      return state.availableNetworks.find((network) => network.id === state.selectedNetworkId);
    },
  },
  strict: debug,
});
