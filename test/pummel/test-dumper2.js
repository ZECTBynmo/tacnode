var assert =require('assert');
var IOWatcher = process.binding('io_watcher').IOWatcher;
var net = require('net');

var ncomplete = 0;

function test (N, size, cb) {
  console.trace();
  var expected = N * size;
  var nread = 0;

  // Create a pipe
  var fds = process.binding('net').pipe();

  // Use writev/dumper to send data down the write end of the pipe, fds[1].
  // This requires a IOWatcher.
  var w = new IOWatcher();
  w.set(fds[1], false, false);

  // The read end, fds[0], will be used to count how much comes through.
  // This sets up a readable stream on fds[0].
  var stream = new net.Stream();
  stream.open(fds[0]);
  stream.readable = true;
  stream.resume();


  // Count the data as it arrives on the read end of the pipe.
  stream.on('data', function (d) {
    nread += d.length;

    if (nread >= expected) {
      assert.ok(nread === expected);
      w.stop();
      console.error("\ndone");
      process.binding('net').close(fds[1]);
    }
  });

  stream.on('close', function () {
    // check to make sure the watcher isn't in the dump queue.
    var x = IOWatcher.dumpQueue;
    while (x.next) {
      assert.ok(x !== w);
      x = x.next;
    }

    ncomplete++;
    if (cb) cb();
  });


  // Create out single 1mb buffer.
  var b = new Buffer(size);
  for (var i = 0; i < size; i++) {
    b[i] = 100;
  }

  // Fill the dumpQueue with N copies of that buffer.
  var x = IOWatcher.dumpQueue;
  while (x.next) {
    x = x.next;
  }
  x.next = w;

  var bucket = w.buckets = { data: b };

  for (var i = 0; i < N-1; i++) {
    bucket = bucket.next = { data: b };
  }
}


function runTests (values) {
  expectedToComplete = values.length;

  function go () {
    if (ncomplete < values.length) {
      var v = values[ncomplete];
      console.log("go %j", v);
      test(v[0], v[1], go);
    }
  }

  go();
}

runTests([ [30, 1000]
         , [4, 10000]
         , [50, 1024*1024]
         ]);


process.on('exit', function () {
  assert.equal(expectedToComplete, ncomplete);
});

