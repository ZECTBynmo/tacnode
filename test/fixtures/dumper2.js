var assert =require('assert');
var net = require('net');
var IOWatcher = process.binding('io_watcher').IOWatcher;

var stdout = new net.Stream(1);
var w = stdout._writeWatcher;

mb = 1024*1024;
b = Buffer(mb);
for (var i = 0; i < mb; i++) {
  b[i] = 100;
}


IOWatcher.dumpQueue.next = w;
var bucket = w.buckets = { data: b };

for (var i = 0; i < 50; i++) {
  bucket = bucket.next = { data: b };
}

/* Total size 50*(1024*1024) = 524288000 */

setTimeout(function () {
  // In the first 10 ms, we haven't pushed out the data.
  assert.ok(null !==  IOWatcher.dumpQueue.next);
}, 10);

process.on('exit', function () {
  assert.ok(!IOWatcher.dumpQueue.next);
});
