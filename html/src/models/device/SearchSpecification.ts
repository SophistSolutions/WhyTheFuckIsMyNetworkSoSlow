export interface ISortBy {
  by: SortFieldEnum;
  ascending?: boolean;
}

export enum SortFieldEnum {
  ADDRESS = 'Address',
  PRIORITY = 'Priority',
  NAME = 'Name',
  TYPE = 'Type',
}

export class SearchSpecification {
  private searchTerms: ISortBy[];
  private compareNetwork?: string;

  constructor(searchCriteria: ISortBy[], compareNetwork?: string) {
    this.searchTerms = searchCriteria;
    this.compareNetwork = compareNetwork;
  }
}
