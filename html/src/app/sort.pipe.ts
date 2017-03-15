import { Pipe, PipeTransform } from '@angular/core';

import { Device }			   from './device'

@Pipe({
	name: 'sort'
})

export class SortPipe implements PipeTransform {

	transform( devices: Device[], sorter: string ) {

		// For when if Devices has not yet been populated
		if (devices == null) return null;
		
		if (sorter == 'name') {
			return devices.sort( function(a:Device,b:Device) {
				var nameA:string = a.name.toUpperCase();
				var nameB:string = b.name.toUpperCase();

				if (nameA < nameB) {
					return -1;
				}
				if (nameA > nameB) {
					return 1;
				}

				return 0;
			});
		}

		if (sorter == 'ipAddress') {

			return devices.sort( function(a:Device,b:Device) {
				var addressA:string = a.ipAddresses[0].toUpperCase();
				var addressB:string = b.ipAddresses[0].toUpperCase();

				if (addressA < addressB) {
					return -1;
				}
				if (addressA > addressB) {
					return 1;
				}

				return 0;
			});
		}

	}

}