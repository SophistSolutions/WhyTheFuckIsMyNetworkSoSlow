import { OperatingSystem } from "@/models/OperatingSystem";

export interface ICurrentMachine {
  operatingSystem: OperatingSystem;
}
export interface IAbout {
  applicationVersion: string;
  components: any;
  currentMachine: ICurrentMachine;
}
