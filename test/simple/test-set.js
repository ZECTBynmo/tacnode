var common = require('../common');
var set = require('_set');
var assert = require('assert');


var s = set.create();

var A = { name: "A" };
var B = { name: "B" };
var C = { name: "C" };
var D = { name: "D" };


s.add(A);

assert.ok(s.contains(A));
assert.ok(!s.contains(B));
assert.ok(!s.contains(C));
assert.ok(!s.contains(D));

s.remove(A);

assert.ok(!s.contains(A));
assert.ok(!s.contains(B));
assert.ok(!s.contains(C));
assert.ok(!s.contains(D));

s.add(A);
s.add(B);


assert.ok(s.contains(A));
assert.ok(s.contains(B));
assert.ok(!s.contains(C));
assert.ok(!s.contains(D));
