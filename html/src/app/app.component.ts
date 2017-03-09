import { Component, OnInit }  from '@angular/core';

import { Device }             from './device';
import { DataService }        from './data.service';


@Component({
  selector: 'my-app',
  template: 
	   `<div class="container-fluid">

	        <div class="row">

	          	<div class="col-md-3 well well-lg text-center unround" id="deviceList">
	            	<div *ngFor="let device of devices | orderBy : 'name' ">
	            		<device-sidebar *ngIf="device.important==true"
							[device]="device"
							[selectedDeviceID]="selectedDeviceID"
							(click)="onSelect(device)">
						</device-sidebar>
					</div>

					<hr>

	            	<div *ngFor="let device of devices | orderBy : 'name' ">
	            		<device-sidebar *ngIf="device.important==false"
							[device]="device"
							[selectedDeviceID]="selectedDeviceID"
							(click)="onSelect(device)">
						</device-sidebar>
					</div>
	          	</div>
	          
	            <div class="col-md-9 text-center well well-lg unround" id="networkTopology"></div>
	          
	        </div>

	    </div>`,
	    
	styles: [`

	    #deviceList {
	        overflow-y:scroll;
	        height: 93vh;
	        margin-bottom; 0px;
	        padding-top: 0px;
	        background:#acacac;
	        border-color:#5A5A5A;
      	}

      	#networkTopology {
      		height: 93vh;
      		background:#acacac;
	        border-color:#5A5A5A;
      	}

	    hr {
		    display: block;
		    margin-left: auto;
		    margin-right: auto;
		    border-style: double;
		    border-width: 8px 0px 8px 0px;
	    }
	`],
	providers: [DataService]
})

export class AppComponent implements OnInit {
	devices: Device[];
	selectedDeviceID: string;

	constructor(private dataService: DataService) { }
	
	ngOnInit() {

		this.dataService.fetchData().subscribe(
			(data) => this.devices = data
		);

		// Refreshes devices every 5 seconds
		setInterval(() => {
			this.dataService.fetchData().subscribe(
				(data) => this.devices = data
			);
		}, 1000 * 5);

	}
	
	onSelect(device: Device): void {
		// If already selected changes the selectedDeviceID to null to deselect
		if (this.selectedDeviceID == device.ipAddresses[0]) {
			this.selectedDeviceID = null;
		}
		else {
    		this.selectedDeviceID = device.ipAddresses[0];
		}
		
  	}
}




