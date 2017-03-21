import { NgModule }        from '@angular/core';
import { BrowserModule }   from '@angular/platform-browser';
import { HttpModule }      from '@angular/http';

import { AppComponent }           from './app.component';
import { DeviceSidebarComponent } from './device-sidebar.component';
import { SortPipe }				  from './sort.pipe';

@NgModule({
  imports:      [ BrowserModule,
  				  HttpModule ],
  declarations: [ AppComponent,
  				  DeviceSidebarComponent,
  				  SortPipe ],
  bootstrap:    [ AppComponent ]
})

export class AppModule { }
