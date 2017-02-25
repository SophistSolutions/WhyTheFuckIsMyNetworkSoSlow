import { NgModule }        from '@angular/core';
import { BrowserModule }   from '@angular/platform-browser';
import { HttpModule }      from '@angular/http';

import { AppComponent }           from './app.component';
import { DeviceSidebarComponent } from './device-sidebar.component';
import { DeviceDetailsComponent } from './device-details.component';

@NgModule({
  imports:      [ BrowserModule,
  				  HttpModule ],
  declarations: [ AppComponent,
  				  DeviceSidebarComponent,
  				  DeviceDetailsComponent ],
  bootstrap:    [ AppComponent ]
})

export class AppModule { }
