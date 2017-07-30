<template>
  <div class="network-visualizer"> <!-- this style should be universal -->
      <nav class="navbar navbar-inverse unround">
            <div class="navbar-header">
                <div class="navbar-brand">{{title}}</div>
            </div>
<!--             <div class="nav navbar-nav navbar-right">
                <li><a href="#" class="dropdown-toggle" data-toggle="dropdown"><span class="glyphicon glyphicon-menu-hamburger" style="font-size:1.2em;"></span></a></li>
                <ul class="dropdown-menu">
                    <li><a href="#">Normal</a></li>
                    <li class="active"><a href="#">Active</a></li>
                    <li><a href="#">Normal</a></li>
                </ul>
            </div> -->
    </nav>

    <div class="container-fluid">

        <div class="row">

            <div class="col-md-3 well well-lg text-center unround" id="deviceList">
<!--                <div *ngFor="let device of (importantDevices | sortBy: 'name')">
                  <device-sidebar *ngIf="device.important==true"
              [device]="device"
              [selectedDeviceID]="selectedDeviceID"
              (click)="onSelect(device)">
            </device-sidebar>
          </div>

          <hr>

                <div *ngFor="let device of (unimportantDevices | sortBy: 'name')">
                  <device-sidebar *ngIf="device.important==false"
              [device]="device"
              [selectedDeviceID]="selectedDeviceID"
              (click)="onSelect(device)">
            </device-sidebar>
          </div> -->
              </div>
            
              <div class="col-md-9 text-center well well-lg unround" id="networkTopology"></div>
            
          </div>

      </div>
    <!-- <my-app><p style="margin:15px 0px 0px 20px;">Loading AppComponent content here ...</p></my-app> -->
  </div>
</template>

<script>
export default {
  name: 'network-visualizer',
  data () {
    return {
      title: 'Network Visualizer'
    }
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>

  .network-visualizer {
    background-color: #007ea7;
    text-align: left;
    margin-bottom: 0px;
  }

  .navbar-brand {
    font-size:150%;
    letter-spacing: 4px;
  }

  h1 {
    color: #369;
    font-family: Arial, Helvetica, sans-serif;
    font-size: 250%;
  }

  #deviceInformation {
    min-height: 120px;
    height: 20vh;
  }

  .well {
    margin-bottom:0px;
  }

  .row {
    margin-bottom: 0px;
  }

  .rounded {
    border-radius: 15px;
  }

  .unround {
    border-radius: 0px;
  }


</style>
