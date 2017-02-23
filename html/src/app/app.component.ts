import { Component, OnInit }  from '@angular/core';

import { Device }             from './device';
import { DataService }        from './data.service';


@Component({
  selector: 'my-app',
  template: 
	   `<div class="container-fluid">

	        <device-details
				[selectedDevice]="selectedDevice">
			</device-details>

	        <div class="row">

	          	<div class="col-md-3 well well-lg text-center" id="deviceList">
	            	<device-sidebar *ngFor="let device of devices"
						[device]="device"
						[class.selected]="device === selectedDevice"
						(click)="onSelect(device)">
					</device-sidebar>
	          	</div>
	          
	            <div class="col-md-9 text-center well well-lg">NETWORK TOPOLOGY</div>
	          
	        </div>

	    </div>`,
	    
	styles: [`
	    .selected {
	      background-color: #CFD8DC !important;
	      color: white;
	    }
	`],
	providers: [DataService]
})

export class AppComponent implements OnInit {
	devices: Device[];
	selectedDevice: Device;

	constructor(private dataService: DataService) { }
	
	ngOnInit() {
		this.dataService.fetchData().subscribe(
			(data) => this.devices = data
		);
	}
	
	onSelect(device: Device): void {
    	this.selectedDevice = device;
  	}
}




