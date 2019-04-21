import Vue from "vue";
import Vuex from "vuex";

import { INetwork } from "@/models/network/INetwork";

import * as mutations from "./mutations";

import { IDevice } from "@/models/device/IDevice";
import { IDeviceQuery } from "@/models/device/query/IDeviceQuery";
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
    devicesQuery: {} as IDeviceQuery,
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
    setAboutInfo(state, aboutInfo: IAbout) {
      Vue.set(state, mutations.ROOT_SET_ABOUT_INFO, aboutInfo);
    },
    setDevicesQuery(state, deviceQuery: IDeviceQuery) {
      Vue.set(state, mutations.ROOT_SET_DEVICES_QUERY, deviceQuery);
    },
  },
  actions: {
    async fetchAvailableNetworks({commit}) {
      commit("setAvailableNetworks", await fetchNetworks());
    },
    async fetchAboutInfo({commit}) {
      commit("setAboutInfo", await fetchAboutInfo());
    },
    async fetchDevices({commit}) {
      commit("setDevices", await fetchDevices());
    },
    setSelectedNetwork({commit}, networkId: string) {
      commit("setSelectedNetwork", networkId);
    },
    setDevicesQuery({commit}, deviceQuery: IDeviceQuery) {
      commit("setDevicesQuery", deviceQuery);
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
    getAboutInfo: (state) => {
      return state.about;
    },
    getSelectedNetworkObject: (state) => {
      return state.availableNetworks.find((network) => network.id === state.selectedNetworkId);
    },
    getDevicesQuery: (state) => {
      return state.devicesQuery;
    },
  },
  strict: debug,
});
