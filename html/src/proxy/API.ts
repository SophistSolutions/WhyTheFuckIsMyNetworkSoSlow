import { DateTime } from 'luxon';

import { IDevice } from '../models/device/IDevice';
import {
  ISortBy,
  SearchSpecification,
} from '../models/device/SearchSpecification';
import { IAbout } from '../models/IAbout';
import { INetwork } from '../models/network/INetwork';
import { INetworkInterface } from '../models/network/INetworkInterface';

import { gRuntimeConfiguration } from 'boot/configuration';

import { Logger } from '../utils/Logger';
import { IDateTimeRange } from 'src/models/common/IDateTimeRange';

import { kCompileTimeConfiguration } from '../config/config';

// On Chrome, CORS frequently fails - not sure why. In chrome debugger, Access-Control-Allow-Origin
// lines not showing up but do in other browsers (edge)
// --LGP 2022-11-24
//const kFetchOptions_: RequestInit = { mode: 'no-cors' };
// I BELIEVE this was fixed by:
//      https://github.com/SophistSolutions/Stroika/commit/cf5a8c8537b0f954729a0911cea51c333ad428de
const kFetchOptions_: RequestInit = {};

function throwIfError_(response: Response): Response {
  if (response.status >= 400 && response.status < 500) {
    throw new Error('Client Request Error from the Server');
  }
  if (response.status >= 500 && response.status < 600) {
    throw new Error(`Server Error ${response.status}`);
  }
  if (!response.ok) {
    throw new Error(`Server Error status: ${response.status}, type:${response.type}`);
  }
  return response;
}

export async function fetchNetworks(): Promise<string[]> {
  try {
    const response: Response = await fetch(
      `${gRuntimeConfiguration.API_ROOT}/api/v1/networks`,
      kFetchOptions_
    );
    throwIfError_(response);
    const data = await response.json()as string[];
    // should validate array of strings (using json schema)
    if (!Array.isArray(data)) {
      throw new Error('Server Data Format Error');
    }
    return data;
  } catch (e) {
    Logger.error(e);
    throw e;
  }
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
  } else {
    d.name = ''; // So we don't fail later - can assume d.name valid string (but should debug any data where we get this)
    // @tod only log if Vue.config.devtools???
    if (kCompileTimeConfiguration.DEBUG_MODE) {
      console.log(`got bad device record with no names, and id=${d.id}`);
    }
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
  try {
    const response: Response = await fetch(
      `${gRuntimeConfiguration.API_ROOT}/api/v1/networks/${id}`,
      kFetchOptions_
    );
    throwIfError_(response);
    const data = await response.json() as INetwork;
    // Could enhance validation (using json schema)
    if (typeof data !== 'object') {
      throw new Error('Server Data Format Error');
    }
    jsonPatch2INetwork_(data);
    return data;
  } catch (e) {
    Logger.error(e);
    throw e;
  }
}

export async function fetchAllActiveNetworkInterfaces(): Promise<
  string[]
> {
  try {
    const response: Response = await fetch(
      `${gRuntimeConfiguration.API_ROOT}/api/v1/network-interfaces`,
      kFetchOptions_
    );
    throwIfError_(response);
    const data = await response.json() as string[];
    return data;
  } catch (e) {
    Logger.error(e);
    throw e;
  }
}

export async function fetchNetworkInterface(
  id: string
): Promise<INetworkInterface> {
  try {
    const response: Response = await fetch(
      `${gRuntimeConfiguration.API_ROOT}/api/v1/network-interfaces/${id}`,
      kFetchOptions_
    );
    throwIfError_(response);
    const data = await response.json() as INetworkInterface;  
    // Could enhance validation (using json schema)
    if (typeof data !== 'object') {
      throw new Error('Server Data Format Error');
    }
    return data;
  } catch (e) {
    Logger.error(e);
    throw e;
  }
}

export async function rescanDevice(deviceID: string): Promise<void> {
  try {
    const response: Response = await fetch(
      `${gRuntimeConfiguration.API_ROOT}/api/v1/operations/scan/FullRescan?deviceID=${deviceID}`,
      kFetchOptions_
    );
    throwIfError_(response);
  } catch (e) {
    Logger.error(e);
    throw e;
  }
}

async function patchCallHelper_(url: string, ops: object[]): Promise<void>
{
  try {
    const response: Response = await fetch(
      url,
      {...kFetchOptions_,
        method: "PATCH",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify(ops),
      }
    );
    throwIfError_(response);
  } catch (e) {
    Logger.error(e);
    throw e;
  }
}

export async function patchDeviceUserProps_name(id: string, newName: string|null): Promise<void> {
  const ops=[];
  if (newName) {
    ops.push ({op: 'add', path:"/userOverrides/name",value:newName});
  }
  else {
    ops.push ({op: 'remove', path:"/userOverrides/name"});
  }
  return await patchCallHelper_(`${gRuntimeConfiguration.API_ROOT}/api/v1/devices/${id}`, ops);
}

export async function patchDeviceUserProps_notes(id: string, newNotes: string|null): Promise<void> {
  const ops=[];
  if (newNotes) {
    ops.push ({op: 'add', path:"/userOverrides/notes",value:newNotes});
  }
  else {
    ops.push ({op: 'remove', path:"/userOverrides/notes"});
  }
  return await patchCallHelper_(`${gRuntimeConfiguration.API_ROOT}/api/v1/devices/${id}`, ops);
}

export async function patchNetworkUserProps_name(id: string, newName: string|null): Promise<void> {
  const ops=[];
  if (newName) {
    ops.push ({op: 'add', path:"/userOverrides/name",value:newName});
  }
  else {
    ops.push ({op: 'remove', path:"/userOverrides/name"});
  }
  return await patchCallHelper_(`${gRuntimeConfiguration.API_ROOT}/api/v1/networks/${id}`, ops);
}

export async function patchNetworkUserProps_notes(id: string, newNotes: string|null): Promise<void> {
  const ops=[];
  if (newNotes) {
    ops.push ({op: 'add', path:"/userOverrides/notes",value:newNotes});
  }
  else {
    ops.push ({op: 'remove', path:"/userOverrides/notes"});
  }
  return await patchCallHelper_(`${gRuntimeConfiguration.API_ROOT}/api/v1/networks/${id}`, ops);
}


export async function fetchDevices(
  searchCriteria?: ISortBy
): Promise<string[]> {
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
  const searchSpecs = new SearchSpecification(searchSpecification);
  const url = searchCriteria
    ? `${gRuntimeConfiguration.API_ROOT}/api/v1/devices?sort=${encodeURI(
        JSON.stringify(searchSpecs)
      )}`
    : `${gRuntimeConfiguration.API_ROOT}/api/v1/devices`;
  try {
    const response: Response = await fetch(url, kFetchOptions_);
    throwIfError_(response);
    const data = await response.json();
    // should validate array of strings (using json schema)
    if (!Array.isArray(data)) {
      throw new Error('Server Data Format Error');
    }
    return data;
  } catch (e) {
    Logger.error(e);
    throw e;
  }
}

export async function fetchDevice(id: string): Promise<IDevice> {
  try {
    const response: Response = await fetch(`${gRuntimeConfiguration.API_ROOT}/api/v1/devices/${id}`, kFetchOptions_);
    throwIfError_(response);
    const data = await response.json() as IDevice; // could embellish validation here
    jsonPatch2IDevice_(data);
    return data;
  } catch (e) {
    Logger.error(e);
    throw e;
  }
}

export async function fetchAboutInfo(): Promise<IAbout> {
  try {
    const response: Response = await fetch(`${gRuntimeConfiguration.API_ROOT}/api/v1/about`, kFetchOptions_);
    throwIfError_(response);
    const data = await response.json() as IAbout; // could embellish validation here
    return data;
  } catch (e) {
    Logger.error(e);
    throw e;
  }
}
