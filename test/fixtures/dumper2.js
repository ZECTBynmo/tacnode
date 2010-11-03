var assert =require('assert');
var IOWatcher = process.binding('io_watcher').IOWatcher;
var net = require('net');


mb = 1024*1024;
N = 50;
expected = N * mb;
nread = 0;

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
  process.stdout.write(".");

  if (nread >= expected) {
    //w.stop();
    // stream.destroy();
    console.error("\ndone");
    process.binding('net').close(fds[1]);
  }
});


// Create out single 1mb buffer.
b = Buffer(mb);
for (var i = 0; i < mb; i++) {
  b[i] = 100;
}

// Fill the dumpQueue with N copies of that buffer.
IOWatcher.dumpQueue.next = w;
var bucket = w.buckets = { data: b };

for (var i = 0; i < N-1; i++) {
  bucket = bucket.next = { data: b };
}




process.on('exit', function () {
  console.log("nread: %d", nread);
  assert.equal(expected, nread);
});

