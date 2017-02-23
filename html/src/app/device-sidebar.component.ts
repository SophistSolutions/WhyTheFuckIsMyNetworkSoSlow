import { Component, Input }  from '@angular/core';

import { Device }	         from './device';

@Component ({
	selector: 'device-sidebar',
  	template: 
  	   `<div class="well well-sm signal-strong rounded">
			<div class="media">
				<div class="media-left">
				    <img src={{device.image}} alt="device-image" class="media-object img-rounded" style="max-width:100px; height: auto;">
				</div>
				<div class="media-body text-right">
					<h4 class="media-heading text-success">{{device.name}} <span class="glyphicon glyphicon-ok-sign"></span></h4>
					<p class="">{{device.type}}</p>								
					<p>{{device.ipv4}}</p>
					<p>{{device.ipv6}}</p>						
					<p>-{{device.signalStrength}}dB <span class="glyphicon glyphicon-signal"></span></p>
				</div>
			</div>
		</div>`
})

export class DeviceSidebarComponent {
	
	@Input() device: Device;

}