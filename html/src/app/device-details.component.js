"use strict";
var __decorate = (this && this.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (this && this.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};
var core_1 = require('@angular/core');
var device_1 = require('./device');
var DeviceDetailsComponent = (function () {
    function DeviceDetailsComponent() {
    }
    __decorate([
        core_1.Input(), 
        __metadata('design:type', device_1.Device)
    ], DeviceDetailsComponent.prototype, "selectedDevice", void 0);
    DeviceDetailsComponent = __decorate([
        core_1.Component({
            selector: 'device-details',
            template: "<div class=\"row media well well-lg\" id=\"deviceInformation\">\n\t  \t    <div *ngIf=\"selectedDevice\">\n\t\t        <div class=\"media-left\">\n\t\t        \t<img src=\"{{selectedDevice.image}}\" alt=\"device-image\" class=\"media-object img-rounded\" style=\"width:100px\">\n\t\t        </div>\n\t\t        <div class=\"media-body text-left\">\n\t\t            <div class=\"col-md-3\">\n\t\t                <h2>{{selectedDevice.name}}</h2>\n\t\t                <p><span class=\"label label-success\">Connected <span class=\"glyphicon glyphicon-ok-sign\"></span></span></p>                         \n\t\t            </div>\n\n\t\t            <div class=\"col-md-4\">\n\t\t                <p>Network           = {{selectedDevice.network}}</p>\n\t\t                <p>Ping (google.com) = 18.02 ms</p>\n\t\t            </div>\n\t\t            \n\t\t            <div class=\"col-md-5\">\n\t\t                <p>Network Mask       = {{selectedDevice.networkMask}}</p>\n\t\t                <p>Network Strength  <span class=\"glyphicon glyphicon-signal\"></span> = -{{selectedDevice.signalStrength}}dB</p>\n\t\t            </div>    \n\t\t        </div>\n\t        </div>\n\t    </div>"
        }), 
        __metadata('design:paramtypes', [])
    ], DeviceDetailsComponent);
    return DeviceDetailsComponent;
}());
exports.DeviceDetailsComponent = DeviceDetailsComponent;
//# sourceMappingURL=device-details.component.js.map