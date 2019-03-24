
import { OperatingSystem } from "@/models/OperatingSystem";

export interface IAbout {
    applicationVersion: string;
    components: any;
    operatingSystem: OperatingSystem;
}
