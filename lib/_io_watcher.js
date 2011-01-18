var assert = process.assert;

var set = require('_set');

var ioAlloc = process.binding('io_watcher').ioAlloc;
var ioFree = process.binding('io_watcher').ioFree;
var ioStop = process.binding('io_watcher').ioStop;
var ioStart = process.binding('io_watcher').ioStart;
var ioSet = process.binding('io_watcher').ioSet;


// We keep a reference to each active watcher so they are not GCed.
var activeWatchers = set.create();

function IOWatcher(object) {
  this.object = object;
  this.pointer = ioAlloc(object);
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
  activeWatchers.add(this);
};


IOWatcher.prototype.stop = function() {
  assert(this.pointer);
  ioStop(this.pointer);
  activeWatchers.remove(this);
};


IOWatcher.prototype.free = function() {
  ioFree(this.pointer);
  this.pointer = null;
  activeWatchers.remove(this);
};
