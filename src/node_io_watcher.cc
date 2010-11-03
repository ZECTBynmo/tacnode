// Copyright 2009 Ryan Dahl <ry@tinyclouds.org>
#include <node_io_watcher.h>

#include <node.h>
#include <node_buffer.h>
#include <v8.h>


#include <sys/uio.h> /* writev */
#include <errno.h>

#include <assert.h>

namespace node {

using namespace v8;

static ev_prepare dumper;
static Persistent<Object> dump_queue;

Persistent<FunctionTemplate> IOWatcher::constructor_template;
Persistent<String> callback_symbol;

static Persistent<String> next_sym;
static Persistent<String> ondrain_sym;
static Persistent<String> onerror_sym;
static Persistent<String> data_sym;
static Persistent<String> offset_sym;
static Persistent<String> buckets_sym;


void IOWatcher::Initialize(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(IOWatcher::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("IOWatcher"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "start", IOWatcher::Start);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "stop", IOWatcher::Stop);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "set", IOWatcher::Set);

  Local<Function> io_watcher = constructor_template->GetFunction();
  target->Set(String::NewSymbol("IOWatcher"), io_watcher);

  callback_symbol = NODE_PSYMBOL("callback");

  next_sym = NODE_PSYMBOL("next");
  ondrain_sym = NODE_PSYMBOL("ondrain");
  onerror_sym = NODE_PSYMBOL("onerror");
  buckets_sym = NODE_PSYMBOL("buckets");
  offset_sym = NODE_PSYMBOL("offset");
  data_sym = NODE_PSYMBOL("data");


  ev_prepare_init(&dumper, IOWatcher::Dump);
  ev_prepare_start(EV_DEFAULT_UC_ &dumper);
  // Need to make sure that Dump runs *after* all other prepare watchers -
  // in particular the next tick one.
  ev_set_priority(&dumper, EV_MINPRI);
  ev_unref(EV_DEFAULT_UC);

  dump_queue = Persistent<Object>::New(Object::New());
  io_watcher->Set(String::NewSymbol("dumpQueue"), dump_queue);
}


void IOWatcher::Callback(EV_P_ ev_io *w, int revents) {
  IOWatcher *io = static_cast<IOWatcher*>(w->data);
  assert(w == &io->watcher_);
  HandleScope scope;

  Local<Value> callback_v = io->handle_->Get(callback_symbol);
  if (!callback_v->IsFunction()) {
    io->Stop();
    return;
  }

  Local<Function> callback = Local<Function>::Cast(callback_v);

  TryCatch try_catch;

  Local<Value> argv[2];
  argv[0] = Local<Value>::New(revents & EV_READ ? True() : False());
  argv[1] = Local<Value>::New(revents & EV_WRITE ? True() : False());

  callback->Call(io->handle_, 2, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }
}


//
//  var io = new process.IOWatcher();
//  process.callback = function (readable, writable) { ... };
//  io.set(fd, true, false);
//  io.start();
//
Handle<Value> IOWatcher::New(const Arguments& args) {
  if (!args.IsConstructCall()) {
    return FromConstructorTemplate(constructor_template, args);
  }

  HandleScope scope;
  IOWatcher *s = new IOWatcher();
  s->Wrap(args.This());
  return args.This();
}


Handle<Value> IOWatcher::Start(const Arguments& args) {
  HandleScope scope;
  IOWatcher *io = ObjectWrap::Unwrap<IOWatcher>(args.Holder());
  io->Start();
  return Undefined();
}


Handle<Value> IOWatcher::Stop(const Arguments& args) {
  HandleScope scope;
  IOWatcher *io = ObjectWrap::Unwrap<IOWatcher>(args.Holder());
  io->Stop();
  return Undefined();
}


void IOWatcher::Start() {
  if (!ev_is_active(&watcher_)) {
    ev_io_start(EV_DEFAULT_UC_ &watcher_);
    Ref();
  }
}


void IOWatcher::Stop() {
  if (ev_is_active(&watcher_)) {
    ev_io_stop(EV_DEFAULT_UC_ &watcher_);
    Unref();
  }
}


Handle<Value> IOWatcher::Set(const Arguments& args) {
  HandleScope scope;

  IOWatcher *io = ObjectWrap::Unwrap<IOWatcher>(args.Holder());

  if (!args[0]->IsInt32()) {
    return ThrowException(Exception::TypeError(
          String::New("First arg should be a file descriptor.")));
  }

  int fd = args[0]->Int32Value();

  if (!args[1]->IsBoolean()) {
    return ThrowException(Exception::TypeError(
          String::New("Second arg should boolean (readable).")));
  }

  int events = 0;

  if (args[1]->IsTrue()) events |= EV_READ;

  if (!args[2]->IsBoolean()) {
    return ThrowException(Exception::TypeError(
          String::New("Third arg should boolean (writable).")));
  }

  if (args[2]->IsTrue()) events |= EV_WRITE;

  assert(!io->watcher_.active);
  ev_io_set(&io->watcher_, fd, events);

  return Undefined();
}

#define KB 1024

/*
 * A large javascript object structure is built up in net.js. The function
 * Dump is called at the end of each iteration, before select() is called,
 * to push all the data out to sockets.
 *
 * The structure looks like this:
 *
 * IOWatcher.dumpQueue -> W -> W -> W -> W
 *                        |    |    |    |
 *                        o    o    o    o
 *                        |    |         |
 *                        o    o         o
 *                             |         |
 *                             o         o
 *                                       |
 *                                       o
 *
 * Where the 'W' nodes are IOWatcher instances associated with a particular
 * socket. The 'o' nodes are little javascript objects with a 'data'
 * member. 'data' is either a string or buffer. E.G.
 *   o = { data: "hello world" }
 *
 */

// To enable this debug output, add '-DDUMP_DEBUG' to CPPFLAGS
// in 'build/c4che/default.cache.py' and 'make clean all'
#ifdef DUMP_DEBUG
#define DEBUG_PRINT(fmt,...) \
  fprintf(stderr, "(dump:%d) " fmt "\n",  __LINE__, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt,...)
#endif


void IOWatcher::Dump(EV_P_ ev_prepare *watcher, int revents) {
  assert(revents == EV_PREPARE);
  assert(watcher == &dumper);

  HandleScope scope;

#define IOV_SIZE 10000
  static struct iovec iov[IOV_SIZE];

  // Loop over all 'fd objects' in the dump queue. Each object stands for a
  // socket that has stuff to be written out.
  Local<Value> writer_node_v;
  Local<Object> writer_node;
  Local<Object> writer_node_last = Local<Object>::New(dump_queue);

  for (writer_node_v = dump_queue->Get(next_sym);
       writer_node_v->IsObject();
       writer_node_v = writer_node->Get(next_sym),
       writer_node_last = writer_node) {

    writer_node = writer_node_v->ToObject();

    IOWatcher *io = ObjectWrap::Unwrap<IOWatcher>(writer_node);
    // stats (just for fun)
    io->dumps_++;
    io->last_dump_ = ev_now(EV_DEFAULT_UC);

    DEBUG_PRINT("Dumping %d", io->watcher_.fd);

    // Number of items we've stored in iov
    int iovcnt = 0;
    // Number of bytes we've stored in iov
    size_t to_write = 0;


    // Offset is only as large as the first buffer of data. (See assert
    // below) Offset > 0 occurs when a previous writev could not entirely
    // drain a bucket.
    size_t offset = 0;
    if (writer_node->Has(offset_sym)) {
      offset = writer_node->Get(offset_sym)->Uint32Value();
    }

    // Loop over all the buckets for this particular socket.
    Local<Value> bucket_v;
    Local<Object> bucket;
    bool first = true;
    unsigned int bucket_index = 0;
    for (bucket_v = writer_node->Get(buckets_sym);
         bucket_v->IsObject() && to_write < 64*KB && iovcnt < IOV_SIZE;
         bucket_v = bucket->Get(next_sym), bucket_index++) {
      bucket = bucket_v->ToObject();

      Local<Value> data_v = bucket->Get(data_sym);
      // net.js will be setting this 'data' value. We can ensure that it is
      // never empty.
      assert(!data_v.IsEmpty());

      Local<Object> buf_object;

      if (data_v->IsString()) {
        // FIXME This is suboptimal - Buffer::New is slow.
        // Also insert v8::String::Pointers() hack here.
        Local<String> s = data_v->ToString();
        buf_object = Local<Object>::New(Buffer::New(s));
        bucket->Set(data_sym, buf_object);
      } else if (Buffer::HasInstance(data_v)) {
        buf_object = data_v->ToObject();
      } else {
        assert(0);
      }

      size_t l = Buffer::Length(buf_object);

      if (first /* ugly */) {
        first = false; // ugly
        assert(offset < l);
        iov[iovcnt].iov_base = Buffer::Data(buf_object) + offset;
        iov[iovcnt].iov_len = l - offset;
      } else {
        iov[iovcnt].iov_base = Buffer::Data(buf_object);
        iov[iovcnt].iov_len = l;
      }
      to_write += iov[iovcnt].iov_len;
      iovcnt++;
    }

    ssize_t written = writev(io->watcher_.fd, iov, iovcnt);

    DEBUG_PRINT("iovcnt: %d, to_write: %ld, written: %ld",
                iovcnt,
                to_write,
                written);

    if (written < 0) {
      // Allow EAGAIN.
      if (errno != EAGAIN) {
        // Emit error event
        if (writer_node->Has(onerror_sym)) {
          Local<Value> callback_v = io->handle_->Get(onerror_sym);
          assert(callback_v->IsFunction());
          Local<Function> callback = Local<Function>::Cast(callback_v);

          Local<Value> argv[1] = { Integer::New(errno) };

          TryCatch try_catch;

          callback->Call(io->handle_, 1, argv);

          if (try_catch.HasCaught()) {
            FatalException(try_catch);
          }
        }
      }
      continue;
    }

    // what about written == 0 ?

    // Now drop the buckets that have been written.
    first = true;
    bucket_index = 0;

    for (bucket_v = writer_node->Get(buckets_sym);
         written > 0 && bucket_v->IsObject();
         bucket_v = bucket->Get(next_sym), bucket_index++) {
      bucket = bucket_v->ToObject();
      assert(written > 0);

      Local<Value> data_v = bucket->Get(data_sym);
      assert(!data_v.IsEmpty());

      // At the moment we're turning all string into buffers
      // so we assert that this is not a string. However, when the
      // "Pointer patch" lands, this assert will need to be removed.
      assert(!data_v->IsString());
      // When the "Pointer patch" lands, we will need to be careful
      // to somehow store the length of strings that we're optimizing on
      // so that it need not be recalculated here. Note the "Pointer patch"
      // will only apply to ASCII strings - UTF8 ones will need to be
      // serialized onto a buffer.
      size_t bucket_len = Buffer::Length(data_v->ToObject());


      if (first) {
        assert(bucket_len > offset);
        first = false;
        DEBUG_PRINT("[%ld] bucket_len: %ld, offset: %ld", bucket_index, bucket_len, offset);
        // Only on the first bucket does the offset matter.
        if (offset + written < bucket_len) {
          // we have not written the entire first bucket
          DEBUG_PRINT("[%ld] Only wrote part of the first buffer. "
                      "setting watcher.offset = %ld",
                      bucket_index,
                      offset + written);

          writer_node->Set(offset_sym,
                          Integer::NewFromUnsigned(offset + written));
          break;
        } else {
          DEBUG_PRINT("[%ld] wrote the whole first bucket. discarding.",
                      bucket_index);
          // We have written the entire bucket, discard it.
          written -= bucket_len - offset;
          writer_node->Set(buckets_sym, bucket->Get(next_sym));

          // Offset is now zero
          writer_node->Set(offset_sym, Integer::NewFromUnsigned(0));
        }
      } else {
        // not first
        DEBUG_PRINT("[%ld] bucket_len: %ld", bucket_index, bucket_len);

        if (static_cast<size_t>(written) < bucket_len) {
          // Didn't write the whole bucket.
          DEBUG_PRINT("[%ld] Only wrote part of the buffer. "
                      "setting watcher.offset = %ld",
                      bucket_index,
                      offset + written);
          writer_node->Set(offset_sym,
                          Integer::NewFromUnsigned(written));
          break;
        } else {
          // Wrote the whole bucket, drop it.
          DEBUG_PRINT("[%ld] wrote the whole bucket. discarding.", bucket_index);
          written -= bucket_len;
          writer_node->Set(buckets_sym, bucket->Get(next_sym));
        }
      }
    }

    // Finished dumping the buckets.
    //
    // If our list of buckets is empty, we can emit 'drain' and forget about
    // this socket. Nothing needs to be done.
    //
    // Otherwise we need to prepare the io_watcher to wait for the interface
    // to become writable again.

    if (writer_node->Get(buckets_sym)->IsUndefined()) {
      // Emptied the queue for this socket.
      // Don't wait for it to become writable.
      io->Stop();

      DEBUG_PRINT("Stop watcher %d", io->watcher_.fd);

      // Drop the writer_node from the list.
      writer_node_last->Set(next_sym, writer_node->Get(next_sym));

      // Emit drain event
      if (writer_node->Has(ondrain_sym)) {
        Local<Value> callback_v = io->handle_->Get(ondrain_sym);
        assert(callback_v->IsFunction());
        Local<Function> callback = Local<Function>::Cast(callback_v);

        TryCatch try_catch;

        callback->Call(io->handle_, 0, NULL);

        if (try_catch.HasCaught()) {
          FatalException(try_catch);
        }
      }

    } else {
      io->Start();
      DEBUG_PRINT("Started watcher %d", io->watcher_.fd);
    }
  }
}




}  // namespace node
