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
var device_service_1 = require('./device.service');
var AppComponent = (function () {
    function AppComponent(deviceService) {
        this.deviceService = deviceService;
    }
    AppComponent.prototype.getDevices = function () {
        var _this = this;
        this.deviceService.getDevices().then(function (devices) { return _this.devices = devices; });
    };
    AppComponent.prototype.ngOnInit = function () {
        this.getDevices();
    };
    AppComponent.prototype.onSelect = function (device) {
        this.selectedDevice = device;
    };
    AppComponent = __decorate([
        core_1.Component({
            selector: 'my-app',
            template: "<div class=\"container-fluid\">\n\n\t        <device-details\n\t\t\t\t[selectedDevice]=\"selectedDevice\">\n\t\t\t</device-details>\n\n\t        <div class=\"row\">\n\n\t          \t<div class=\"col-md-3 well well-lg text-center\" id=\"deviceList\">\n\t            \t<device-sidebar *ngFor=\"let device of devices\"\n\t\t\t\t\t\t[device]=\"device\"\n\t\t\t\t\t\t[class.selected]=\"device === selectedDevice\"\n\t\t\t\t\t\t(click)=\"onSelect(device)\">\n\t\t\t\t\t</device-sidebar>\n\t          \t</div>\n\t          \n\t            <div class=\"col-md-9 text-center well well-lg\">NETWORK TOPOLOGY</div>\n\t          \n\t        </div>\n\n\t    </div>",
            styles: ["\n\t    .selected {\n\t      background-color: #CFD8DC !important;\n\t      color: white;\n\t    }\n\t"],
            providers: [device_service_1.DeviceService]
        }), 
        __metadata('design:paramtypes', [device_service_1.DeviceService])
    ], AppComponent);
    return AppComponent;
}());
exports.AppComponent = AppComponent;
//# sourceMappingURL=app.component.js.map