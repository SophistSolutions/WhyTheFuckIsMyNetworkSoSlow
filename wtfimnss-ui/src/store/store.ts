import Vue from 'vue';
import Vuex from 'vuex';

import { INetwork } from '@/models/Network/INetwork';

import * as mutations from './mutations';

import { fetchNetworks } from '@/proxy/API';

Vue.use(Vuex);

const debug = process.env.NODE_ENV !== 'production';

// TODO make this the root store - have other stores be props of state. Seperate by logical groups
export default new Vuex.Store({
  state: {
    availableNetworks: [] as INetwork[],
    selectedNetworkId: {} as string,
  },
  mutations: {
    setAvailableNetworks(state, networks: INetwork[]) {
      Vue.set(state, mutations.ROOT_SET_AVAILABLE_NETWORKS, networks);
      // state[mutations.ROOT_SET_AVAILABLE_NETWORK] = networks;
    },
    setSelectedNetwork(state, networkId: string) {
      Vue.set(state, mutations.ROOT_SET_SELECTED_NETWORK_ID, networkId);
    },
  },
  actions: {
    async fetchAvailableNetworks({commit}) {
      commit('setAvailableNetworks', await fetchNetworks());
    },
    setSelectedNetwork({commit}, networkId: string) {
      commit('setSelectedNetwork', networkId);
    },
  },
  getters: {
    getAvailableNetworks: (state) => {
      return state.availableNetworks;
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
