var L = require('_linklist');
function Set() {
  this.list = {};
  L.init(this.list);
}
exports.Set = Set;


exports.create = function() {
  return new Set();
};


Set.prototype.add = function(obj) {
  L.init(obj);
  L.append(this.list, obj);
  obj.__setMembership = this;
};


Set.prototype.remove = function(obj) {
  L.remove(obj);
  obj.__setMembership = null;
};


Set.prototype.contains = function(obj) {
  return obj.__setMembership === this;
};
