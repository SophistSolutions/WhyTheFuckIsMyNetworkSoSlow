export interface IGeographicLocation {
    city: string
    coordinates: ICoordinates
    countryCode: string
    postalCode: string
    regionCode: string
}

interface ICoordinates {
    latitude: number
    longitude: number
}
