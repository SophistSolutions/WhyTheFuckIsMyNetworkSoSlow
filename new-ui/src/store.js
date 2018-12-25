export const store = {
  state: {
    values: []
  },
  addValue: function(newNumber) {
    this.state.values.push(newNumber);
  },
  removeValue: function(newNumber) {
    var index = this.state.values.indexOf(newNumber);
    if (index > -1) { this.state.values.splice(index, 1); }
  }
};
