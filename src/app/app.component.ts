import { Component, OnInit }  from '@angular/core';

import { Device }             from './device';
import { DeviceService }      from './device.service';


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
	providers: [DeviceService]
})

export class AppComponent implements OnInit {
	devices: Device[];
	selectedDevice: Device;

	constructor(private deviceService: DeviceService) { }
	
	getDevices(): void {
		this.deviceService.getDevices().then(devices => this.devices = devices);
	}
	ngOnInit(): void {
		this.getDevices();
	}

	onSelect(device: Device): void {
    	this.selectedDevice = device;
  	}
}




