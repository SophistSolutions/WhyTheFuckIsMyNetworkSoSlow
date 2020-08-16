import Vue from "vue";
import Vuex from "vuex";

import { INetwork } from "@/models/network/INetwork";

import * as mutations from "./mutations";

import { IDevice } from "@/models/device/IDevice";
import { ISortBy } from "@/models/device/SearchSpecification";
import { IAbout } from "@/models/IAbout";
import { fetchAboutInfo, fetchDevices, fetchNetworks } from "@/proxy/API";

Vue.use(Vuex);

const debug = process.env.NODE_ENV !== "production";

// TODO make this the root store - have other stores be props of state. Seperate by logical groups
export default new Vuex.Store({
  state: {
    about: undefined as IAbout | undefined,
    availableNetworks: [] as INetwork[],
    selectedNetworkId: {} as string,
    devices: [] as IDevice[],
    searchString: "" as string,
  },
  mutations: {
    setAvailableNetworks(state, networks: INetwork[]) {
      Vue.set(state, mutations.ROOT_SET_AVAILABLE_NETWORKS, networks);
      // state[mutations.ROOT_SET_AVAILABLE_NETWORK] = networks;
    },
    setDevices(state, devices: IDevice[]) {
      Vue.set(state, mutations.ROOT_SET_DEVICES, devices);
    },
    setSelectedNetwork(state, networkId: string) {
      Vue.set(state, mutations.ROOT_SET_SELECTED_NETWORK_ID, networkId);
    },
    setSearchString(state, searchString: string) {
      Vue.set(state, mutations.ROOT_SET_SEARCH_STRING_ID, searchString);
    },
    setAboutInfo(state, aboutInfo: IAbout) {
      Vue.set(state, mutations.ROOT_SET_ABOUT_INFO, aboutInfo);
    },
  },
  actions: {
    async fetchAvailableNetworks({ commit }) {
      commit("setAvailableNetworks", await fetchNetworks());
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
    setSearchString({ commit }, searchString: string) {
      commit("searchString", searchString);
    },
  },
  getters: {
    getAvailableNetworks: (state) => {
      return state.availableNetworks;
    },
    getDevices: (state) => {
      return state.devices;
    },
    getSelectedNetworkId: (state) => {
      return state.selectedNetworkId;
    },
    getSearchString: (state) => {
      return state.searchString;
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
