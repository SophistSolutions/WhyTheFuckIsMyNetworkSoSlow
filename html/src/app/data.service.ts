import { Injectable } from '@angular/core';
import { Http }       from '@angular/http';

/* Allows .map function to interpret json file */
import { Observable } from 'rxjs/Observable';
import 'rxjs/add/operator/map';

@Injectable()
export class DataService {
	
	constructor(private http: Http) {}
	
	fetchData(){
		return this.http.get('./Devices.json').map(
			(res)  => res.json() 
		);
	}

}	