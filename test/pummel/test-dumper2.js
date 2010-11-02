var common = require("../common");
var assert = require("assert");
var spawn = require('child_process').spawn;

var script = require('path').join(common.fixturesDir, "dumper2.js");

var child = spawn(process.execPath, [script]);

var size = 0;
child.stdout.on('data', function (d) {
  assert.equal(100, d[0]);
  process.stdout.write('.');
  size += d.length;
});

child.stderr.setEncoding('utf8');
child.stderr.on('data', function (d) {
  console.error(d);
});

child.on('exit', function (status) {
  assert.equal(0, status);
});

process.on('exit', function () {
  assert.equal(50*(1024*1024), size);
});

