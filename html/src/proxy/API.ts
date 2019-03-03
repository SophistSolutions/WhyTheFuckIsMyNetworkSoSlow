import { INetwork } from '@/models/network/INetwork'
import { IDevice } from '@/models/device/IDevice'

import { API_ROOT, API_NETWORKS_PATH, API_DEVICES_PATH } from '@/config'


export async function fetchNetworks(): Promise<INetwork[]> {
    return fetch(API_ROOT + API_NETWORKS_PATH)
    .then((response) => response.json())
    .then((data) => {
        return data
    })
    .catch((error) => console.error(error))
}

export async function fetchDevices(): Promise<IDevice[]> {
    return fetch(API_ROOT + API_DEVICES_PATH)
    .then((response) => response.json())
    .then((data) => {
        return data
    })
    .catch((error) => console.error(error))
}

// TODO could instead do this but not modular enough for future
// export default {
//     fetchNetworks,
// };
