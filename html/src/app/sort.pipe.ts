import { Pipe, PipeTransform } from '@angular/core';

import { Device }			   from './device'

@Pipe({
	name: 'sort'
})

export class SortPipe implements PipeTransform {

	transform( devices: Device[] ) {

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
		
		/*return devices.sort(function(a, b) {
		  var nameA = a.name.toUpperCase(); // ignore upper and lowercase
		  var nameB = b.name.toUpperCase(); // ignore upper and lowercase
		  if (nameA < nameB) {
		    return -1;
		  }
		  if (nameA > nameB) {
		    return 1;
		  }

		  // names must be equal
		  return 0;
		}); */

	}

}