import { Component, Input, OnInit }  from '@angular/core';

import { DataService } 		 from './data.service';
import { Device }	         from './device';

@Component ({
	selector: 'device-sidebar',
  	template: 
  	   `<div class="panel panel-success signal-strong text-left rounded">

	  	   	<div class="panel-heading">
	  	   		<p class="media-heading text-success overflow-ellipsis">{{device.name}} <span *ngIf="device.important" class="glyphicon glyphicon-ok-sign"></span></p>
	  	   	</div>
	  		<div class="panel-body">
				<div class="media">
					<div class="media-left">
						<div *ngIf="device.type">
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
						<div *ngIf="!device.type">
							<img src="http://placehold.it/150x150" alt="placeholder" class="media-object img-rounded" style="max-width:70px; height: auto; display: inline-block;">
						</div>
					</div>
					<div class="media-body text-right">
						<ul>
							<li>{{device.type}}</li>
							<li>{{device.ipAddress}}</li>
							<div *ngIf="device.signalStrength">
								<li>-{{device.signalStrength}}dB <span class="glyphicon glyphicon-signal"></span></li>
							</div>
						</ul>
					</div>
				</div>
			</div>

		</div>`,
	styles: [`

		.panel {
    		margin: 10px 0px 10px 0px;
    	}

    	.overflow-ellipsis {
    		margin: 0px 0px 0px 0px;
    		text-overflow: ellipsis;
    		white-space: nowrap;
    		overflow: hidden;
    	}

    	li {
    		list-style-type: none;
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