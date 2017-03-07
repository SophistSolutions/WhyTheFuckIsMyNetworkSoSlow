import { NgModule }        from '@angular/core';
import { BrowserModule }   from '@angular/platform-browser';
import { HttpModule }      from '@angular/http';
import { Ng2OrderModule } from 'ng2-order-pipe';

import { AppComponent }           from './app.component';
import { DeviceSidebarComponent } from './device-sidebar.component';
import { DeviceDetailsComponent } from './device-details.component';

@NgModule({
  imports:      [ BrowserModule,
  				  HttpModule,
  				  Ng2OrderModule ],
  declarations: [ AppComponent,
  				  DeviceSidebarComponent,
  				  DeviceDetailsComponent ],
  bootstrap:    [ AppComponent ]
})

export class AppModule { }
