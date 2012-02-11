#!node

var marked = require('marked');
var fs = require('fs');

// parse the args.
// Don't use nopt or whatever for this.  It's simple enough.

var args = process.argv.slice(2);
var format = 'json';
var template = null;
var input = null;

args.forEach(function (arg) {
  if (!arg.match(/^\-\-/)) {
    input = arg;
  } else if (arg.match(/^\-\-format=/)) {
    format = arg.replace(/^\-\-format=/, '');
  } else if (arg.match(/^\-\-template=/)) {
    template = arg.replace(/^\-\-template=/, '');
  }
})

// If we didn't get an input, then read from stdin.
if (input) {
  console.error('Input file = %s', input);
  fs.readFile(input, 'utf8', function(er, input) {
    if (er) throw er;
    next(input);
  });
} else {
  var i = '';
  var stdin = process.openStdin();
  var StringDecoder = require('string_decoder').StringDecoder;
  var sd = new StringDecoder('utf8');
  stdin.on('data', function(chunk) {
    i += sd.write(chunk);
  });
  stdin.on('end', function() {
    next(i);
  });
}

function next(input) {
  switch (format) {
    case 'json':
      require('./json.js')(input, function(er, obj) {
        console.log(JSON.stringify(obj, null, 2));
        if (er) throw er;
      });
      break;

    case 'html':
      require('./html.js')(input, template, function(er, html) {
        if (er) throw er;
        console.log(html);
      });
      break;

    default:
      throw new Error('Invalid format: ' + format);
  }
}
