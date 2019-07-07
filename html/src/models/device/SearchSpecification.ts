export interface ISortBy {
    by: SortFieldEnum;
}

export enum SortFieldEnum {
    ADDRESS = "Address",
    PRIORITY = "Priority",
    NAME = "Name",
    TYPE = "Type",
}

export class SearchSpecification {
    private searchTerms: ISortBy[];
    private compareNetwork?: string;

    constructor(searchTerms: SortFieldEnum[], compareNetwork?: string) {

        this.searchTerms = searchTerms.map((field: SortFieldEnum) => {
            return {by: field};
        });

        this.compareNetwork = compareNetwork;
    }
}
