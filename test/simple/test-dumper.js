var assert =require('assert');
var IOWatcher = process.binding('io_watcher').IOWatcher;
var errnoException = process.binding('net').errnoException;
var net = require('net');

var ncomplete = 0;

function test (N, b, cb) {
  //console.trace();
  var expected = N * b.length;
  var nread = 0;

  // Create a pipe
  var fds = process.binding('net').pipe();

  // Use writev/dumper to send data down the write end of the pipe, fds[1].
  // This requires a IOWatcher.
  var w = new IOWatcher();
  w.set(fds[1], false, false);

  var ndrain = 0;
  w.ondrain = function () {
    ndrain++;
  }

  var nerror = 0;
  w.onerror = function (errno) {
    throw errnoException(errno);
    nerror++;
  }

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
      assert.equal(1, ndrain);
      assert.equal(0, nerror);
      w.stop();
      console.error("done. wrote %d bytes\n", nread);
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

    assert.equal(null, w.next);

    ncomplete++;
    if (cb) cb();
  });


  // Create out single 1mb buffer.

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
      console.log("test N=%d, size=%d", v[0], v[1].length);
      test(v[0], v[1], go);
    }
  }

  go();
}

runTests([ [30, Buffer(1000)]
         , [4, Buffer(10000)]
         , [1, "hello world\n"]
         , [50, Buffer(1024*1024)]
         , [500, Buffer(40960+1)]
         , [500, Buffer(40960-1)]
         , [500, Buffer(40960)]
         , [500, Buffer(1024*1024+1)]
         , [50000, "hello world\n"]
         ]);


process.on('exit', function () {
  assert.equal(expectedToComplete, ncomplete);
});

