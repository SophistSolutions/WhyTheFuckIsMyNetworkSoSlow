import Vue from 'vue';
import Vuex from 'vuex';

import { INetwork } from '@/models/Network/INetwork';

import * as mutations from './store/mutations';

import { fetchNetworks } from '@/proxy/API';

Vue.use(Vuex);

const debug = process.env.NODE_ENV !== 'production';

// TODO make this the root store - have other stores be props of state. Seperate by logical groups
export default new Vuex.Store({
  state: {
    availableNetworks: [] as INetwork[],
    // TODO make this an ID - better to denormalize data instead of maintaining 2 seperate copies of same data
    selectedNetwork: {} as INetwork,
  },
  mutations: {
    setAvailableNetworks(state, networks: INetwork[]) {
      Vue.set(state, mutations.ROOT_SET_AVAILABLE_NETWORKS, networks);
      // state[mutations.ROOT_SET_AVAILABLE_NETWORK] = networks;
    },
    setSelectedNetwork(state, network: INetwork) {
      Vue.set(state, mutations.ROOT_SET_SELECTED_NETWORK, network);
    },
  },
  actions: {
    async fetchAvailableNetworks({commit}) {
      commit('setAvailableNetworks', await fetchNetworks());
    },
// make direct api call from inside actions
  },
  getters: {
    getAvailableNetworks: (state) => {
      return state.availableNetworks;
    },
  },
  strict: debug,
});
