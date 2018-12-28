import { INetwork } from '@/models/Network/INetwork';

import { API_ROOT, API_NETWORKS_PATH } from '@/config';

export async function fetchNetworks(): Promise<INetwork[]> {
    return fetch(API_ROOT + API_NETWORKS_PATH)
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
