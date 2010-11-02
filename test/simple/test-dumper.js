var assert =require('assert');
var IOWatcher = process.binding('io_watcher').IOWatcher;
var net = require('net');

var stdout = new net.Stream(1);
var w = stdout._writeWatcher;

var bucketA = { data: "hello " };
var bucketB = { data: "world\n" };
var bucketC = { data: Buffer("and a buffer\n") };

IOWatcher.dumpQueue.next = w;
w.buckets = bucketA;
bucketA.next = bucketB;
bucketB.next = bucketC;

setTimeout(function () {
  assert.equal(null, IOWatcher.dumpQueue.next);
  console.log("100");
}, 100);

setTimeout(function () {
  assert.equal(null, IOWatcher.dumpQueue.next);
  console.log("1000");
}, 1000);
