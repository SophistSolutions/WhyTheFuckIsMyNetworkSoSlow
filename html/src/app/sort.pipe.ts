import { Pipe, PipeTransform } from '@angular/core';

import { Device }			   from './device'

@Pipe({
	name: 'sort'
})

export class SortPipe implements PipeTransform {

	transform( devices: Device[] ) {

		// For when if Devices has not yet been populated
		if (devices == null) return null;
		
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

}