import { Component, Input }  from '@angular/core';

import { Device }	         from './device';

@Component ({
	selector: 'device-details',
  	template: 
  	   `<div class="row media well well-lg" id="deviceInformation">
	  	    <div *ngIf="selectedDevice">
		        <div class="media-left">
		        	<div *ngIf="selectedDevice.type=='Phone'">
		        		<img src="./images/phone.png" alt="phone-image" class="media-object img-rounded" style="width:100px">
		        	</div>
		        	<div *ngIf="selectedDevice.type=='WAP'">
		        		<img src="./images/WAP.png" alt="WAP-image" class="media-object img-rounded" style="width:100px">
		        	</div>
		        	<div *ngIf="selectedDevice.type=='Laptop'">
		        		<img src="./images/laptop.png" alt="laptop-image" class="media-object img-rounded" style="width:100px">
		        	</div>
		        	<div *ngIf="selectedDevice.type=='Router'">
		        		<img src="./images/router.png" alt="router-image" class="media-object img-rounded" style="width:100px">
		        	</div>
		        	<div *ngIf="selectedDevice.type=='Chromecast'">
		        		<img src="./images/chromecast.png" alt="chromecast-image" class="media-object img-rounded" style="width:100px">
		        	</div>
		        	<div *ngIf="selectedDevice.type=='Printer'">
		        		<img src="./images/printer.png" alt="printer-image" class="media-object img-rounded" style="width:100px">
		        	</div>
		        	<div *ngIf="selectedDevice.type=='Roku'">
		        		<img src="./images/chromecast.png" alt="roku-image" class="media-object img-rounded" style="width:100px">
		        	</div>
		        	<div *ngIf="selectedDevice.type=='Tablet'">
		        		<img src="./images/laptop.png" alt="tablet-image" class="media-object img-rounded" style="width:100px">
		        	</div>
		        </div>
		        <div class="media-body text-left">
		            <div class="col-md-3">
		                <h3>{{selectedDevice.name}}</h3>
		                <div *ngIf="selectedDevice.important">
		                	<p><span class="label label-success">Watch List <span class="glyphicon glyphicon-eye-open"></span></span></p>      
		             	</div>                   
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