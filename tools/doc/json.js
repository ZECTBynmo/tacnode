module.exports = doJSON;

// Take the lexed input, and return a JSON-encoded object
// A module looks like this: https://gist.github.com/1777387

var marked = require('marked');
var root = {};
var stack = [root];
var depth = 0;
var current = root;
var state = null;

function doJSON(input, cb) {
  var lexed = marked.lexer(input);
  lexed.forEach(function (tok) {
    var type = tok.type;
    var text = tok.text;

    // <!-- type = module -->
    // This is for cases where the markdown semantic structure is lacking.
    var meta;
    if (type === 'paragraph' &&
        (meta = text.match(/^<!--([^=]+)=([^\-])+-->\n*$/))) {
      current[meta[1].trim()] = meta[2].trim();
      return
    }

    if (type === 'heading' &&
        !text.trim().toLowerCase().match(/^example/)) {
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
      state = 'AFTERHEADING';
      return;
    } // heading

    // Immediately after a heading, we can expect the following
    //
    // { type: 'code', text: 'Stability: ...' },
    //
    // a list: starting with list_start, ending with list_end,
    // maybe containing other nested lists in each item.
    //
    // If one of these isnt' found, then anything that comes between
    // here and the next heading should be parsed as the desc.
    var stability
    if (state === 'AFTERHEADING') {
      if (type === 'code' &&
          (stability = text.match(/^Stability: ([0-5])(?:\s*-\s*)?(.*)$/))) {
        current.stability = parseInt(stability[1], 10);
        current.stabilityText = stability[2].trim();
        return;
      } else if (type === 'list_start' && !tok.ordered) {
        state = 'AFTERHEADING_LIST';
        current.list = current.list || [];
        current.list.push(tok);
        current.list.level = 1;
      } else {
        current.desc = current.desc || [];
        current.desc.push(tok);
        state = 'DESC';
      }
      return;
    }

    if (state === 'AFTERHEADING_LIST') {
      current.list.push(tok);
      if (type === 'list_start') {
        current.list.level++;
      } else if (type === 'list_end') {
        current.list.level--;
      }
      if (current.list.level === 0) {
        state = 'AFTERHEADING';
        processList(current);
      }
      return;
    }

    current.desc = current.desc || [];
    current.desc.push(tok);

  });

  // finish any sections left open
  while (root !== (current = stack.pop())) {
    finishSection(current, stack[stack.length - 1]);
  }

  return cb(null, root)
}


// go from something like this:
// [ { type: 'list_item_start' },
//   { type: 'text',
//     text: '`settings` Object, Optional' },
//   { type: 'list_start', ordered: false },
//   { type: 'list_item_start' },
//   { type: 'text',
//     text: 'exec: String, file path to worker file.  Default: `__filename`' },
//   { type: 'list_item_end' },
//   { type: 'list_item_start' },
//   { type: 'text',
//     text: 'args: Array, string arguments passed to worker.' },
//   { type: 'text',
//     text: 'Default: `process.argv.slice(2)`' },
//   { type: 'list_item_end' },
//   { type: 'list_item_start' },
//   { type: 'text',
//     text: 'silent: Boolean, whether or not to send output to parent\'s stdio.' },
//   { type: 'text', text: 'Default: `false`' },
//   { type: 'space' },
//   { type: 'list_item_end' },
//   { type: 'list_end' },
//   { type: 'list_item_end' },
//   { type: 'list_end' } ]
// to something like:
// [ { name: 'settings',
//     type: 'object',
//     optional: true,
//     settings:
//      [ { name: 'exec',
//          type: 'string',
//          desc: 'file path to worker file',
//          default: '__filename' },
//        { name: 'args',
//          type: 'array',
//          default: 'process.argv.slice(2)',
//          desc: 'string arguments passed to worker.' },
//        { name: 'silent',
//          type: 'boolean',
//          desc: 'whether or not to send output to parent\'s stdio.',
//          default: 'false' } ] } ]

function processList(section) {
  var list = section.list;
  var values = [];
  var current;
  var stack = [];

  // for now, *just* build the heirarchical list
  list.forEach(function(tok) {
    var type = tok.type;
    if (type === 'space') return;
    if (type === 'list_item_start') {
      if (!current) {
        var n = {};
        values.push(n);
        current = n;
      } else {
        current.options = current.options || [];
        stack.push(current);
        var n = {};
        current.options.push(n);
        current = n;
      }
      return;
    } else if (type === 'list_item_end') {
      if (!current) {
        throw new Error('invalid list - end without current item\n' +
                        JSON.stringify(tok) + '\n' +
                        JSON.stringify(list));
      }
      current = stack.pop();
    } else if (type === 'text') {
      if (!current) {
        throw new Error('invalid list - text without current item\n' +
                        JSON.stringify(tok) + '\n' +
                        JSON.stringify(list));
      }
      current.textRaw = current.textRaw || '';
      current.textRaw += tok.text + ' ';
    }
  });

  // shove the name in there for properties, since they are always
  // just going to be the value etc.
  if (section.type === 'property') {
    values[0].textRaw = '`' + section.name + '` ' + values[0].textRaw;
  }

  // now pull the actual values out of the text bits.
  values.forEach(parseListItem);

  // Now figure out what this list actually means.
  // depending on the section type, the list could be different things.

  switch (section.type) {
    case 'method':
      // each item is an argument, unless the name is 'return',
      // in which case it's the return value.
      section.params = values.filter(function(v) {
        if (v.name === 'return') {
          section.return = v;
          return false;
        }
        return true;
      });
      break;

    case 'property':
      // there should be only one item, which is the value.
      // copy the data up to the section.
      var value = values[0];
      delete value.name;
      section.typeof = value.type;
      delete value.type;
      Object.keys(value).forEach(function(k) {
        section[k] = value[k];
      });
      break;

    case 'event':
      // event: each item is an argument.
      section.params = values;
      break;
  }

  // section.listParsed = values;
  delete section.list;

}


function parseListItem(item) {
  if (item.options) item.options.forEach(parseListItem);
  if (!item.textRaw) return;

  // the goal here is to find the name, type, default, and optional.
  // anything left over is 'desc'
  var text = item.textRaw.trim();
  text = text.replace(/^(Argument|Param)s?\s*:?\s*/i, '');

  text = text.replace(/^, /, '').trim();
  var retExpr = /^Returns?\s*:?\s*/i;
  var ret = text.match(retExpr);
  if (ret) {
    item.name = 'return';
    text = text.replace(retExpr, '');
  } else {
    var nameExpr = /^['`"]?([^'`": ]+)['`"]?\s*:?\s*/;
    var name = text.match(nameExpr);
    if (name) {
      item.name = name[1];
      text = text.replace(nameExpr, '');
    }
  }

  text = text.replace(/^, /, '').trim();
  var defaultExpr = /default\s*[:=]?\s*['"`]?([^, '"`]*)['"`]?/i;
  var def = text.match(defaultExpr);
  if (def) {
    item.default = def[1];
    text = text.replace(defaultExpr, '');
  }

  text = text.replace(/^, /, '').trim();
  var typeExpr =
      /^((?:[a-zA-Z]* )?object|string|bool(?:ean)?|regexp?|null|function)/i;
  var type = text.match(typeExpr);
  if (type) {
    item.type = type[1];
    text = text.replace(typeExpr, '');
  }

  text = text.replace(/^, /, '').trim();
  var optExpr = /^Optional\.|(?:, )?Optional$/;
  var optional = text.match(optExpr);
  if (optional) {
    item.optional = true;
    text = text.replace(optExpr, '');
  }

  text = text.trim();
  if (text) item.desc = text;
}


function finishSection(section, parent) {
  if (!section || !parent) {
    throw new Error('Invalid finishSection call\n'+
                    JSON.stringify(section) + '\n' +
                    JSON.stringify(parent));
  }

  if (!section.type) {
    section.type = 'module';
    section.displayName = section.name;
    section.name = section.name.toLowerCase();
  }

  if (section.desc) {
    section.desc = marked.parser(section.desc);
  }

  if (section.list) {
    processList(section);
  }

  // properties are a bit special.
  // their "type" is the type of object, not "property"
  if (section.properties) {
    section.properties.forEach(function (p) {
      if (p.typeof) p.type = p.typeof;
      else delete p.type;
      delete p.typeof;
    });
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
  var text = section.textRaw = tok.text;
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
