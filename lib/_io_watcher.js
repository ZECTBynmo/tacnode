var assert = process.assert;

var set = require('_set');

var ioAlloc = process.binding('io_watcher').ioAlloc;
var ioFree = process.binding('io_watcher').ioFree;
var ioStop = process.binding('io_watcher').ioStop;
var ioStart = process.binding('io_watcher').ioStart;
var ioSet = process.binding('io_watcher').ioSet;


// We keep a reference to each watcher so they are not GCed.
var allWatchers = set.create();

function IOWatcher(object) {
  this.pointer = ioAlloc(object);
  allWatchers.add(this);
};
exports.IOWatcher = IOWatcher;


IOWatcher.prototype.set = function(fd, readable, writable) {
  assert(this.pointer);
  this.stop();
  ioSet(this.pointer, fd, readable, writable);
};


IOWatcher.prototype.start = function() {
  assert(this.pointer);
  ioStart(this.pointer);
};


IOWatcher.prototype.stop = function() {
  assert(this.pointer);
  ioStop(this.pointer);
};


IOWatcher.prototype.free = function() {
  assert(this.pointer);
  ioStop(this.pointer);
  ioFree(this.pointer);
  this.pointer = null;
  allWatchers.remove(this);
};
