import { NgModule }      from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';

import { AppComponent }  from './app.component';
import { DeviceSidebarComponent } from './device-sidebar.component';
import { DeviceDetailsComponent } from './device-details.component';

@NgModule({
  imports:      [ BrowserModule ],
  declarations: [ AppComponent,
  				  DeviceSidebarComponent,
  				  DeviceDetailsComponent ],
  bootstrap:    [ AppComponent ]
})

export class AppModule { }
