module.exports = doJSON;

// Take the lexed input, and return a JSON-encoded object
// A module looks like this: https://gist.github.com/1777387

var marked = require('marked');
var root = {};
var stack = [root];
var depth = 0;
var current = root;

function doJSON(input, cb) {
  var lexed = marked.lexer(input);
  lexed.forEach(function (tok) {
    var type = tok.type;
    var text = tok.text;

    // <!-- type = module -->
    // This is for cases where the markdown itself is lacking.
    var meta;
    if (type === 'paragraph' &&
        (meta = text.match(/^<!--([^=]+)=([^\-])+-->\n*$/))) {
      current[meta[1].trim()] = meta[2].trim();
      return
    }

    if (type === 'heading') {
      if (Math.abs(tok.depth - depth) > 1) {
        return cb(new Error('Inappropriate heading level\n'+
                            JSON.stringify(tok)));
      }

      // if the level is greater than the current depth,
      // then it's a child, so we should just leave the stack
      // as it is.
      // However, if it's a sibling or higher, then it implies
      // the closure of the other sections that came before.
      // root is always considered the level=0 section,
      // and the lowest heading is 1, so this should always
      // result in having a valid parent node.
      var d = tok.depth;
      while (d <= depth) {
        finishSection(stack.pop(), stack[stack.length - 1]);
        d++;
      }

      current = newSection(tok);
      depth = tok.depth;
      stack.push(current);
      return;
    }

    // { type: 'code', text: 'Stability: 1 - Experimental' },

  });

  // finish any sections left open
  while (root !== (current = stack.pop())) {
    finishSection(current, stack[stack.length - 1]);
  }

  return cb(null, root)
}


function finishSection(section, parent) {
  if (!section || !parent) {
    throw new Error('Invalid finishSection call\n'+
                    JSON.stringify(section) + '\n' +
                    JSON.stringify(parent));
  }

  if (!section.type) {
    section.type = 'module';
    section.name = section.name.toLowerCase();
  }

  var plur;
  if (section.type.slice(-1) === 's') {
    plur = section.type + 'es';
  } else if (section.type.slice(-1) === 'y') {
    plur = section.type.replace(/y$/, 'ies');
  } else {
    plur = section.type + 's';
  }

  parent[plur] = parent[plur] || [];
  parent[plur].push(section);
}



// these parse out the contents of an H# tag
var eventExpr = /^Event:?\s*['"]?([^"']+).*$/i;
var classExpr = /^Class:\s*([^ ]+).*?$/i;
var propExpr = /^(?:property:?\s*)?[^\.]+\.([^ \.\(\)]+)\s*?$/i;
var methExpr = /^(?:method:?\s*)?[^\.]+\.([^ \.\(\)]+)\([^\)]*\)\s*?$/i;

function newSection(tok) {
  var section = {};
  // infer the type from the text.
  var text = tok.text;
  if (text.match(eventExpr)) {
    section.type = 'event';
    section.name = text.replace(eventExpr, '$1');
  } else if (text.match(classExpr)) {
    section.type = 'class';
    section.name = text.replace(classExpr, '$1');
  } else if (text.match(propExpr)) {
    section.type = 'property';
    section.name = text.replace(propExpr, '$1');
  } else if (text.match(methExpr)) {
    section.type = 'method';
    section.name = text.replace(methExpr, '$1');
  } else {
    section.name = text;
  }
  return section;
}
