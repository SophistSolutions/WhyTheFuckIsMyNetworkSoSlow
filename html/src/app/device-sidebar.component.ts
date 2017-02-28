import { Component, Input, OnInit }  from '@angular/core';

import { DataService } 		 from './data.service';
import { Device }	         from './device';

@Component ({
	selector: 'device-sidebar',
  	template: 
  	   `<div class="panel panel-success signal-strong text-left rounded">

	  	   	<div class="panel-heading"><p class="media-heading text-success">{{device.name}} <span class="glyphicon glyphicon-ok-sign"></span></p><span class="glyphicon glyphicon-option-vertical" style="display: inline; float: right;"aria-hidden="true"></span></div>
	  		<div class="panel-body">
				<div class="media">
					<div class="media-left">
					    <div *ngIf="device.type=='Phone'">
					    	<img src="./images/phone.png" alt="phone-image" class="media-object img-rounded" style="max-width:70px; height: auto; display: inline-block;">
					    </div>
					    <div *ngIf="device.type=='WAP'">
					    	<img src="./images/WAP.png" alt="WAP-image" class="media-object img-rounded" style="max-width:70px; height: auto; display: inline-block;">
					    </div>
					    <div *ngIf="device.type=='Laptop'">
					    	<img src="./images/laptop.png" alt="laptop-image" class="media-object img-rounded" style="max-width:70px; height: auto; display: inline-block;">
					    </div>
					    <div *ngIf="device.type=='Router'">
					    	<img src="./images/router.png" alt="router-image" class="media-object img-rounded" style="max-width:70px; height: auto; display: inline-block;">
					    </div>
					    <div *ngIf="device.type=='Chromecast'">
					    	<img src="./images/chromecast.png" alt="chromecast-image" class="media-object img-rounded" style="max-width:70px; height: auto; display: inline-block;">
					    </div>
					    <div *ngIf="device.type=='Printer'">
					    	<img src="./images/printer.png" alt="printer-image" class="media-object img-rounded" style="max-width:70px; height: auto; display: inline-block;">
					    </div>
					    <div *ngIf="device.type=='Roku'">
					    	<img src="./images/chromecast.png" alt="roku-image" class="media-object img-rounded" style="max-width:70px; height: auto; display: inline-block;">
					    </div>
					    <div *ngIf="device.type=='Tablet'">
					    	<img src="./images/laptop.png" alt="tablet-image" class="media-object img-rounded" style="max-width:70px; height: auto; display: inline-block;">
					    </div>
					</div>
					<div class="media-body text-right">
						<p class="">{{device.type}}</p>								
						<p>{{device.ipv4}}</p>
						<p>{{device.ipv6}}</p>						
						<p>-{{device.signalStrength}}dB <span class="glyphicon glyphicon-signal"></span></p>
					</div>
				</div>
			</div>
			
		</div>`,
	styles: [`

		.panel {
    		margin:10px 0px 10px 0px;
    	}

    	.panel-heading p {
    		display: inline-block;
    		font-weight: bold;
    		white-space: nowrap;
    		width: 100px;
    		width:inherit;
    		overflow:hidden
    		text-overflow: ellipsis;
    	}

    	.signal-strong {
    		background-color: #8cff66
    	}

    	.signal-medium {
    		background-color: #ffc266
    	}

    	.signal-weak {
    		background-color: #ff6666;
    	}

    	.signal-dead {
    		background-color: #b3b3b3;
    	}
    `],
    providers: [DataService]
})

export class DeviceSidebarComponent {

	@Input() device: Device;

}