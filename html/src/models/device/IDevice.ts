import { DeviceTypeEnum } from '@/models/device/DeviceTypeEnum'

export interface IDevice {
    id: string
    attachedNetworks: string[]
    name: string
    internetAddresses: string[]
    type?: DeviceTypeEnum
    presentationURL?: string
}
