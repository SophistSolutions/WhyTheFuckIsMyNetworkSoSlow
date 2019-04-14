import { IDevice } from "@/models/device/IDevice";
import { IAbout } from "@/models/IAbout";
import { INetwork } from "@/models/network/INetwork";

import { API_DEVICES_PATH, API_NETWORKS_PATH, API_ROOT } from "@/config";


export async function fetchNetworks(): Promise<INetwork[]> {
    return fetch(API_ROOT + API_NETWORKS_PATH)
    .then((response) => response.json())
    .then((data) => {
        return data;
    })
    .catch((error) => console.error(error));
}

export async function fetchDevices(): Promise<IDevice[]> {
    return fetch(API_ROOT + API_DEVICES_PATH)
    .then((response) => response.json())
    .then((data) => {
        return data;
    })
    .catch((error) => console.error(error));
}

export async function fetchAboutInfo(): Promise<IAbout> {
    return fetch(API_ROOT + "/about")
    .then((response) => response.json())
    .then((data) => {
        return data;
    })
    .catch((error) => console.error(error));
}

// TODO could instead do this but not modular enough for future
// export default {
//     fetchNetworks,
// };
