import Vue from 'vue';
import Vuex from 'vuex';

import { INetwork } from '@/models/network/INetwork';

import * as mutations from './mutations';

import { fetchNetworks, fetchDevices } from '@/proxy/API';
import { IDevice } from '@/models/device/IDevice';

Vue.use(Vuex);

const debug = process.env.NODE_ENV !== 'production';

// TODO make this the root store - have other stores be props of state. Seperate by logical groups
export default new Vuex.Store({
  state: {
    availableNetworks: [] as INetwork[],
    selectedNetworkId: {} as string,
    devices: [] as IDevice[],
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
  },
  actions: {
    async fetchAvailableNetworks({commit}) {
      commit('setAvailableNetworks', await fetchNetworks());
    },
    async fetchDevices({commit}) {
      commit('setDevices', await fetchDevices());
    },
    setSelectedNetwork({commit}, networkId: string) {
      commit('setSelectedNetwork', networkId);
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
    getSelectedNetworkObject: (state) => {
      return state.availableNetworks.find((network) => network.id === state.selectedNetworkId);
    },
  },
  strict: debug,
});
