import { Component, Input }  from '@angular/core';

import { Device }	         from './device';

@Component ({
	selector: 'device-details',
  	template: 
  	   `<div class="row media well well-lg" id="deviceInformation">
	  	    <div *ngIf="selectedDevice">
		        <div class="media-left">
		        	<img src="{{selectedDevice.image}}" alt="device-image" class="media-object img-rounded" style="width:100px">
		        </div>
		        <div class="media-body text-left">
		            <div class="col-md-3">
		                <h2>{{selectedDevice.name}}</h2>
		                <p><span class="label label-success">Connected <span class="glyphicon glyphicon-ok-sign"></span></span></p>                         
		            </div>

		            <div class="col-md-4">
		                <p>Network           = {{selectedDevice.network}}</p>
		                <p>Ping (google.com) = 18.02 ms</p>
		            </div>
		            
		            <div class="col-md-5">
		                <p>Network Mask       = {{selectedDevice.networkMask}}</p>
		                <p>Network Strength  <span class="glyphicon glyphicon-signal"></span> = -{{selectedDevice.signalStrength}}dB</p>
		            </div>    
		        </div>
	        </div>
	    </div>`
})

export class DeviceDetailsComponent {
	
	@Input() selectedDevice: Device;

}