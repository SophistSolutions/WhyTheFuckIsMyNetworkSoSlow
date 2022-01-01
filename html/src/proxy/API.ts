import { IDevice } from '@/models/device/IDevice'
import { ISortBy, SearchSpecification, SortFieldEnum } from '@/models/device/SearchSpecification'
import { IAbout } from '@/models/IAbout'
import { INetwork } from '@/models/network/INetwork'
import { INetworkInterface } from '@/models/network/INetworkInterface'

import { API_ROOT } from '@/config'

import { Logger } from '@/utils/Logger'

export async function fetchNetworks (): Promise<INetwork[]> {
  return fetch(API_ROOT + '/networks?recurse=true')
    .then((response) => response.json())
    .then((data) => {
      return data
    })
    .catch((error) => Logger.error(error))
}

export async function fetchNetwork (id: string): Promise<INetwork> {
  return fetch(API_ROOT + `/networks/${id}`)
    .then((response) => response.json())
    .then((data) => {
      return data
    })
    .catch((error) => Logger.error(error))
}

export async function fetchNetworkInterfaces (): Promise<INetworkInterface[]> {
  return fetch(API_ROOT + '/network-interfaces?recurse=true')
    .then((response) => response.json())
    .then((data) => {
      return data
    })
    .catch((error) => Logger.error(error))
}

export async function rescanDevice (deviceID: string): Promise<void> {
  return fetch(API_ROOT + `/operations/scan/FullRescan?deviceID=${deviceID}`)
    .then((response) => response.json())
    .then((data) => {
      return data
    })
    .catch((error) => Logger.error(error))
}

export async function fetchDevices (searchCriteria?: ISortBy): Promise<IDevice[]> {
  // @todo make these search params depend on parameters,
  // and especially make compareNetwork depend on current active network
  // (and maybe sometimes omit)

  const searchSpecification: ISortBy[] = []

  if (!searchCriteria) {
    searchSpecification.push({ by: SortFieldEnum.ADDRESS, ascending: true })
    searchSpecification.push({ by: SortFieldEnum.TYPE, ascending: true })
    searchSpecification.push({ by: SortFieldEnum.PRIORITY, ascending: true })
  } else {
    searchSpecification.push(searchCriteria)
  }

  // TODO correct hardcoded compareNetwork
  const searchSpecs = new SearchSpecification(searchSpecification, '192.168.244.0/24')

  return fetch(API_ROOT + `/devices?recurse=true&sort=${encodeURI(JSON.stringify(searchSpecs))}`)
    .then((response) => response.json())
    .then((data) => {
      data.forEach((d: any) => {
        // fixup urls that are relative to be relative to the WSAPI
        if (d.icon) {
          d.icon = new URL(d.icon, API_ROOT)
        }
      })
      return data
    })
    .then((data) => {
      return data
    })
    .catch((error) => Logger.error(error))
}

export async function fetchDevice (id: string): Promise<IDevice> {
  return fetch(API_ROOT + `/devices/${id}`)
    .then((response) => response.json())
    .then((data) => {
      // fixup urls that are relative to be relative to the WSAPI
      if (data.icon) {
        data.icon = new URL(data.icon, API_ROOT)
      }
      return data
    })
    .then((data) => {
      return data
    })
    .catch((error) => Logger.error(error))
}

export async function fetchAboutInfo (): Promise<IAbout> {
  return fetch(API_ROOT + '/about')
    .then((response) => response.json())
    .then((data) => {
      return data
    })
    .catch((error) => Logger.error(error))
}

// TODO could instead do this but not modular enough for future
// export default {
//     fetchNetworks,
// };
