import moment from 'moment';

import { IDevice } from '../models/device/IDevice';
import {
  ISortBy,
  SearchSpecification,
  SortFieldEnum,
} from '../models/device/SearchSpecification';
import { IAbout } from '../models/IAbout';
import { INetwork } from '../models/network/INetwork';
import { INetworkInterface } from '../models/network/INetworkInterface';

import { gRuntimeConfiguration } from 'boot/configuration';

import { Logger } from '../utils/Logger';
import { IDateTimeRange } from 'src/models/common/IDateTimeRange';

export async function fetchNetworks(): Promise<INetwork[]> {
  return fetch(`${gRuntimeConfiguration.API_ROOT}/api/v1/networks?recurse=true`)
    .then((response) => response.json())
    .then((data) => {
      //tmphack backward compat -- LGP 2022-07-18
      data.forEach((n: { seen?: IDateTimeRange; lastSeenAt?: Date }) => {
        n.lastSeenAt = n.seen?.upperBound;
      });
      return data;
    })
    .catch((error) => Logger.error(error));
}

export async function fetchNetwork(id: string): Promise<INetwork> {
  return fetch(`${gRuntimeConfiguration.API_ROOT}/api/v1/networks/${id}`)
    .then((response) => response.json())
    .then((data) => {
      //tmphack backward compat -- LGP 2022-07-18
      data.lastSeenAt = data.seen?.upperBound;
      return data;
    })
    .catch((error) => Logger.error(error));
}

export async function fetchNetworkInterfaces(): Promise<INetworkInterface[]> {
  return fetch(
    `${gRuntimeConfiguration.API_ROOT}/api/v1/network-interfaces?recurse=true`
  )
    .then((response) => response.json())
    .then((data) => {
      return data;
    })
    .catch((error) => Logger.error(error));
}

export async function rescanDevice(deviceID: string): Promise<void> {
  return fetch(
    `${gRuntimeConfiguration.API_ROOT}/api/v1/operations/scan/FullRescan?deviceID=${deviceID}`
  )
    .then((response) => response.json())
    .then((data) => {
      return data;
    })
    .catch((error) => Logger.error(error));
}

export async function fetchDevices(
  searchCriteria?: ISortBy
): Promise<IDevice[]> {
  // @todo make these search params depend on parameters,
  // and especially make compareNetwork depend on current active network
  // (and maybe sometimes omit)

  const searchSpecification: ISortBy[] = [];

  if (!searchCriteria) {
    // if no criteria specified, let WSAPI return defaults...
    // searchSpecification.push({ by: SortFieldEnum.ADDRESS, ascending: true });
    // searchSpecification.push({ by: SortFieldEnum.TYPE, ascending: true });
    // searchSpecification.push({ by: SortFieldEnum.PRIORITY, ascending: true });
  } else {
    searchSpecification.push(searchCriteria);
  }

  // TODO correct hardcoded compareNetwork
  const searchSpecs = new SearchSpecification(
    searchSpecification,
    '192.168.244.0/24'
  );

  return fetch(
    `${
      gRuntimeConfiguration.API_ROOT
    }/api/v1/devices?recurse=true&sort=${encodeURI(
      JSON.stringify(searchSpecs)
    )}`
  )
    .then((response) => response.json())
    .then((data) => {
      data.forEach((d: { icon: URL | string | null }) => {
        // fixup urls that are relative to be relative to the WSAPI
        if (d.icon) {
          d.icon = new URL(d.icon, gRuntimeConfiguration.API_ROOT);
        }
      });
      data.forEach((d: { seen: { [key: string]: IDateTimeRange } }) => {
        if (d.seen) {
          let mn = null;
          let mx = null;
          for (const [key, value] of Object.entries(d.seen)) {
            if (mn == null) {
              mn = value.lowerBound;
              mx = value.upperBound;
            } else {
              // @todo must fix to map to dates first
              if (moment(value.lowerBound).diff(mn) < 0) {
                mn = value.lowerBound;
              }
              if (moment(value.upperBound).diff(mn) > 0) {
                mx = value.upperBound;
              }
            }
          }
          d.seen.Ever = { lowerBound: mn, upperBound: mx };
        }
        d.lastSeenAt = d.seen?.Ever?.upperBound;
      });
      return data;
    })
    .then((data) => {
      return data;
    })
    .catch((error) => Logger.error(error));
}

export async function fetchDevice(id: string): Promise<IDevice> {
  return fetch(`${gRuntimeConfiguration.API_ROOT}/api/v1/devices/${id}`)
    .then((response) => response.json())
    .then((data) => {
      // fixup urls that are relative to be relative to the WSAPI
      if (data.icon) {
        data.icon = new URL(data.icon, gRuntimeConfiguration.API_ROOT);
      }
      if (data.seen) {
        let mn = null;
        let mx = null;
        for (const [key, value] of Object.entries(data.seen)) {
          if (mn == null) {
            mn = value.lowerBound;
            mx = value.upperBound;
          } else {
            // @todo must fix to map to dates first
            if (moment(value.lowerBound).diff(mn) < 0) {
              mn = value.lowerBound;
            }
            if (moment(value.upperBound).diff(mn) > 0) {
              mx = value.upperBound;
            }
          }
        }
        data.seen.Ever = { lowerBound: mn, upperBound: mx };
      }
      data.lastSeenAt = data.seen?.Ever?.upperBound;
      return data;
    })
    .then((data) => {
      return data;
    })
    .catch((error) => Logger.error(error));
}

export async function fetchAboutInfo(): Promise<IAbout> {
  return fetch(`${gRuntimeConfiguration.API_ROOT}/api/v1/about`)
    .then((response) => response.json())
    .then((data) => {
      return data;
    })
    .catch((error) => Logger.error(error));
}
