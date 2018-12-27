import { INetwork } from '@/models/Network/INetwork';

export async function fetchNetworks(): Promise<INetwork[]> {
    return fetch('http://192.168.244.187:8080/networks?recurse=true')
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
