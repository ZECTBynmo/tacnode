var fs = require('fs');
var marked = require('marked');
var path = require('path');

module.exports = toHTML;

function toHTML(input, filename, template, cb) {
  var lexed = marked.lexer(input);
  fs.readFile(template, 'utf8', function(er, template) {
    if (er) return cb(er);
    render(lexed, filename, template, cb);
  });
}

function render(lexed, filename, template, cb) {
  // get the section
  var section = getSection(lexed);

  filename = path.basename(filename, '.markdown');

  // generate the table of contents.
  // this mutates the lexed contents in-place.
  buildToc(lexed, filename, function(er, toc) {
    if (er) return cb(er);

    template = template.replace(/__FILENAME__/g, filename);

    template = template.replace(/__SECTION__/g, section);

    template = template.replace(/__VERSION__/g, process.version);

    template = template.replace(/__TOC__/g, toc);

    // content has to be the last thing we do with
    // the lexed tokens, because it's destructive.
    content = marked.parser(lexed);
    template = template.replace(/__CONTENT__/g, content);

    cb(null, template);
  });
}


// section is just the first heading
function getSection(lexed) {
  var section = '';
  for (var i = 0, l = lexed.length; i < l; i++) {
    var tok = lexed[i];
    if (tok.type === 'heading') return tok.text;
  }
  return '';
}


function buildToc(lexed, filename, cb) {
  var indent = 0;
  var toc = [];
  var depth = 0;
  lexed.forEach(function(tok) {
    if (tok.type !== 'heading') return;
    if (Math.abs(tok.depth - depth) > 1) {
      return cb(new Error('Inappropriate heading level\n' +
                          JSON.stringify(tok)));
    }

    depth = tok.depth;
    var id = getId(filename + '_' + tok.text.trim());
    toc.push(new Array((depth - 1) * 2 + 1).join(' ') +
             '* <a href="#' + id + '">' +
             tok.text + '</a>');
    tok.text += '<span><a class="mark" href="#' + id + '" ' +
                'id="' + id + '">#</a></span>';
  });

  toc = marked.parse(toc.join('\n'));
  cb(null, toc);
}

var idCounter = 0;
function getId(text) {
  text = text.toLowerCase();
  text = text.replace(/[^a-z0-9]+/g, '_');
  text = text.replace(/^_+|_+$/, '');
  text = text.replace(/^([^a-z])/, '_$1');
  text += '_' + (idCounter ++);
  return text;
}

