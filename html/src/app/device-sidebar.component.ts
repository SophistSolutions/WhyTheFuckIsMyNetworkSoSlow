import { Component, Input, OnInit }  from '@angular/core';

import { DataService } 		 from './data.service';
import { Device }	         from './device';

@Component ({
	selector: 'device-sidebar',
  	template: 
  	   `<div class="well well-sm signal-strong rounded">
			<div class="media">
				<div class="media-left">
				    <img src={{device.image}} alt="device-image" class="media-object img-rounded" style="max-width:70px; height: auto; display: inline-block;">
				</div>
				<div class="media-body text-right">
					<div style="word-break: break-all;"><h4 class="media-heading text-success">{{device.name}} <span class="glyphicon glyphicon-ok-sign"></span></h4></div>
					<p class="">{{device.type}}</p>								
					<p>{{device.ipv4}}</p>
					<p>{{device.ipv6}}</p>						
					<p>-{{device.signalStrength}}dB <span class="glyphicon glyphicon-signal"></span></p>
				</div>
			</div>
		</div>`,
	styles: [`
	  .well {
        margin:10px 0px 10px 0px;
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