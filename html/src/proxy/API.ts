import { DateTime } from 'luxon';

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

// On Chrome, CORS frequently fails - not sure why. In chrome debugger, Access-Control-Allow-Origin
// lines not showing up but do in other browsers (edge)
// --LGP 2022-11-24
//const kFetchOptions_: RequestInit = { mode: 'no-cors' };
const kFetchOptions_: RequestInit = {};

export async function fetchNetworks(): Promise<INetwork[]> {
  return fetch(
    `${gRuntimeConfiguration.API_ROOT}/api/v1/networks?recurse=true`,
    kFetchOptions_
  )
    .then((response) => response.json())
    .then((data) => {
      return data;
    })
    .catch((error) => Logger.error(error));
}

function jsonPatch2IDateRange_(d: IDateTimeRange) {
  if (d.upperBound) {
    d.upperBound = DateTime.fromISO(
      d.upperBound as unknown as string
    ).toJSDate();
  }
  if (d.lowerBound) {
    d.lowerBound = DateTime.fromISO(
      d.lowerBound as unknown as string
    ).toJSDate();
  }
}
// munge json in place to valid INetwork
function jsonPatch2INetwork_(n: INetwork) {
  if (n.seen) {
    jsonPatch2IDateRange_(n.seen);
  }
}
// munge json in place to valid IDevice
function jsonPatch2IDevice_(d: IDevice) {
  // fixup urls that are relative to be relative to the WSAPI
  if (d.icon) {
    d.icon = new URL(d.icon, gRuntimeConfiguration.API_ROOT);
  }
  // Compute the derived name field
  if (d.names && d.names.length >= 1) {
    d.name = d.names[0].name;
  }
  // Compute the seen.Ever 'virtual field'
  if (d.seen) {
    let mn: Date | undefined;
    let mx: Date | undefined;
    for (const [key, value] of Object.entries(d.seen)) {
      jsonPatch2IDateRange_(value);
      if (mn == undefined) {
        mn = value.lowerBound;
      } else if (
        value.lowerBound != undefined &&
        value.lowerBound.getTime() - mn.getTime() < 0
      ) {
        mn = value.lowerBound;
      }
      if (mx == undefined) {
        mx = value.upperBound;
      } else if (
        value.upperBound != undefined &&
        value.upperBound.getTime() - mx.getTime() > 0
      ) {
        mx = value.upperBound;
      }
    }
    d.seen.Ever = { lowerBound: mn, upperBound: mx };
  }
}

export async function fetchNetwork(id: string): Promise<INetwork> {
  return fetch(
    `${gRuntimeConfiguration.API_ROOT}/api/v1/networks/${id}`,
    kFetchOptions_
  )
    .then((response) => response.json())
    .then((data) => {
      jsonPatch2INetwork_(data);
      return data;
    })
    .catch((error) => Logger.error(error));
}

export async function fetchAllActiveNetworkInterfaces(): Promise<
  INetworkInterface[]
> {
  return fetch(
    `${gRuntimeConfiguration.API_ROOT}/api/v1/network-interfaces?recurse=true`,
    kFetchOptions_
  )
    .then((response) => response.json())
    .then((data) => {
      return data;
    })
    .catch((error) => Logger.error(error));
}

export async function fetchNetworkInterface(
  id: string
): Promise<INetworkInterface> {
  return fetch(
    `${gRuntimeConfiguration.API_ROOT}/api/v1/network-interfaces/${id}`,
    kFetchOptions_
  )
    .then((response) => response.json())
    .then((data) => {
      return data;
    })
    .catch((error) => Logger.error(error));
}

export async function rescanDevice(deviceID: string): Promise<void> {
  return fetch(
    `${gRuntimeConfiguration.API_ROOT}/api/v1/operations/scan/FullRescan?deviceID=${deviceID}`,
    kFetchOptions_
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
    )}`,
    kFetchOptions_
  )
    .then((response) => response.json())
    .then((data) => {
      data.forEach((d: IDevice) => {
        jsonPatch2IDevice_(d);
      });
      return data;
    })
    .then((data) => {
      return data;
    })
    .catch((error) => Logger.error(error));
}

export async function fetchDevice(id: string): Promise<IDevice> {
  return fetch(
    `${gRuntimeConfiguration.API_ROOT}/api/v1/devices/${id}`,
    kFetchOptions_
  )
    .then((response) => response.json())
    .then((data) => {
      jsonPatch2IDevice_(data);
      return data;
    })
    .then((data) => {
      return data;
    })
    .catch((error) => Logger.error(error));
}

export async function fetchAboutInfo(): Promise<IAbout> {
  return fetch(`${gRuntimeConfiguration.API_ROOT}/api/v1/about`, kFetchOptions_)
    .then((response) => response.json())
    .then((data) => {
      return data;
    })
    .catch((error) => Logger.error(error));
}
