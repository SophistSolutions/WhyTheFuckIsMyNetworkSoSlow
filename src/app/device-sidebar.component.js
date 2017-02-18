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
var DeviceSidebarComponent = (function () {
    function DeviceSidebarComponent() {
    }
    __decorate([
        core_1.Input(), 
        __metadata('design:type', device_1.Device)
    ], DeviceSidebarComponent.prototype, "device", void 0);
    DeviceSidebarComponent = __decorate([
        core_1.Component({
            selector: 'device-sidebar',
            template: "<div class=\"well well-sm signal-strong rounded\">\n\t\t\t<div class=\"media\">\n\t\t\t\t<div class=\"media-left\">\n\t\t\t\t    <img src={{device.image}} alt=\"device-image\" class=\"media-object img-rounded\" style=\"max-width:100px; height: auto;\">\n\t\t\t\t</div>\n\t\t\t\t<div class=\"media-body text-right\">\n\t\t\t\t\t<h4 class=\"media-heading text-success\">{{device.name}} <span class=\"glyphicon glyphicon-ok-sign\"></span></h4>\n\t\t\t\t\t<p class=\"\">{{device.type}}</p>\t\t\t\t\t\t\t\t\n\t\t\t\t\t<p>{{device.ipv4}}</p>\n\t\t\t\t\t<p>{{device.ipv6}}</p>\t\t\t\t\t\t\n\t\t\t\t\t<p>-{{device.signalStrength}}dB <span class=\"glyphicon glyphicon-signal\"></span></p>\n\t\t\t\t</div>\n\t\t\t</div>\n\t\t</div>"
        }), 
        __metadata('design:paramtypes', [])
    ], DeviceSidebarComponent);
    return DeviceSidebarComponent;
}());
exports.DeviceSidebarComponent = DeviceSidebarComponent;
//# sourceMappingURL=device-sidebar.component.js.map