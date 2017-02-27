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

					<hr>

					<device-sidebar *ngFor="let device of devices"
						[device]="device"
						[class.selected]="device === selectedDevice"
						(click)="onSelect(device)">
					</device-sidebar>

					<hr>

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

	    hr {
		    display: block;
		    margin-top: 0.5em;
		    margin-bottom: 0.5em;
		    margin-left: auto;
		    margin-right: auto;
		    border-style: inset;
		    border-width: 4px;
	    }
	`],
	providers: [DataService]
})

export class AppComponent implements OnInit {
	devices: Device[];
	selectedDevice: Device;

	constructor(private dataService: DataService) { }
	
	ngOnInit() {

		/* Refreshes devices every 5 seconds */
		setInterval(() => {
			this.dataService.fetchData().subscribe(
				(data) => this.devices = data
			);
		}, 1000 * 5);

		this.dataService.fetchData().subscribe(
			(data) => this.devices = data
		);
	}
	
	onSelect(device: Device): void {
    	this.selectedDevice = device;
  	}
}




