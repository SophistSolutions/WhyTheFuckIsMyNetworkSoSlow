import { OperatingSystem } from "../models/OperatingSystem";

export interface IComponent {
  URL?: string;
  name: string;
  version: string;
}
export interface ICurrentMachine {
  operatingSystem: OperatingSystem;
  machineUptime?: string;
  runQLength?: number;
  totalCPUUsage?: number;
}
export interface ICurrentProcess {
  averageCPUTimeUsed?: number;
  combinedIOReadRate?: number;
  combinedIOWriteRate?: number;
  processUptime?: string;
  workingOrResidentSetSize?: number;
}
export interface IServerInfo {
  componentVersions: IComponent[];
  components: any;
  currentMachine: ICurrentMachine;
  currentProcess: ICurrentProcess;
}
export interface IAbout {
  applicationVersion: string;
  serverInfo: IServerInfo;
}
