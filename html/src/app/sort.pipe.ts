import { Pipe, PipeTransform } from '@angular/core';

import { Device }			   from './device'

@Pipe({
	name: 'sortBy'
})

export class SortPipe implements PipeTransform {

	transform( devices: Device[], sortBy: string ):any {

		// For when if Devices has not yet been populate
		if (devices == null) return null;

		switch (sortBy) {

			default:
				devices.sort( function(a:Device,b:Device) {
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
		 		break;

		 	case 'ipAddress':
		 		devices.sort( function(a:Device,b:Device) {
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
		 		break;

		 	case 'name':
		 		devices.sort( function(a:Device,b:Device) {
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
		 		break;

		 	case 'type':
		 		devices.sort( function(a:Device,b:Device) {
		 		var typeA:string = a.type.toUpperCase();
		 		var typeB:string = b.type.toUpperCase();

		 		if (typeA < typeB) {
		 			return -1;
		 		}
		 		if (typeA > typeB) {
		 			return 1;
		 		}
		 		return 0;
		 		});
		 		break;

		}

		return devices;

	}
}