
/*
 *  Return an array of image-url/label pairs, for the given array of device types.
 */
export function PluralizeNoun(t: string, n: number): string {
 return n == 1? t : (t + t.endsWith("s")? "es": "s");
}
