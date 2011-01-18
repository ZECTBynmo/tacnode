// Copyright 2009 Ryan Dahl <ry@tinyclouds.org>
#include <node_io_watcher.h>

#include <node.h>
#include <v8.h>

#include <assert.h>
#include <stdint.h>  // intptr_t
#include <stdlib.h>

extern char etext;

namespace node {

using namespace v8;

Persistent<FunctionTemplate> IOWatcher::constructor_template;
Persistent<String> callback_symbol;
Persistent<String> on_io_symbol;


static inline Local<Value> AddressToJS(void* address) {
  intptr_t diff = (intptr_t)address - (intptr_t)&etext;
  if (diff < 0x7FFFFFFF && diff > 0) {
    return Integer::New(diff);
  } else {
    return External::New(address);
  }
}


template <class T>
static inline T* JSToAddress(Local<Value> v) {
  if (v->IsExternal()) {
    return static_cast<T*>(External::Unwrap(v));
  } else {
    assert(v->IsInt32());
    return static_cast<T*>((void*)(v->IntegerValue() + &etext));
  }
}


static void IOCallback(EV_P_ ev_io *watcher, int revents) {
  HandleScope scope;

  Persistent<Object> obj = *static_cast<Persistent<Object>*>(watcher->data);

  Local<Value> callback_v = obj->Get(on_io_symbol);
  if (!callback_v->IsFunction()) return;

  Local<Function> callback = Local<Function>::Cast(callback_v);

  TryCatch try_catch;

  Local<Value> argv[4];
  argv[0] = AddressToJS(watcher);
  argv[1] = Local<Value>::New(revents & EV_READ ? True() : False());
  argv[2] = Local<Value>::New(revents & EV_WRITE ? True() : False());

  callback->Call(obj, 3, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }
}


static Handle<Value> IOAlloc(const Arguments& args) {
  HandleScope scope;

  ev_io *io = new ev_io;
  ev_init(io, IOCallback);

  assert(args[0]->IsObject());

  Persistent<Object> *obj_p = new Persistent<Object>();
  *obj_p = Persistent<Object>::New(args[0]->ToObject());
  io->data = obj_p;

  return scope.Close(AddressToJS(io));
}


static Handle<Value> IOFree(const Arguments& args) {
  HandleScope scope;
  ev_io *io = JSToAddress<ev_io>(args[0]);
  delete io;
  return Undefined();
}


static Handle<Value> IOSet(const Arguments& args) {
  HandleScope scope;

  ev_io *io = JSToAddress<ev_io>(args[0]);

  if (!args[1]->IsInt32()) {
    return ThrowException(Exception::TypeError(
          String::New("First arg should be a file descriptor.")));
  }

  int fd = args[1]->Int32Value();

  if (!args[2]->IsBoolean()) {
    return ThrowException(Exception::TypeError(
          String::New("Second arg should boolean (readable).")));
  }

  int events = 0;

  if (args[2]->IsTrue()) events |= EV_READ;

  if (!args[3]->IsBoolean()) {
    return ThrowException(Exception::TypeError(
          String::New("Third arg should boolean (writable).")));
  }

  if (args[3]->IsTrue()) events |= EV_WRITE;

  ev_io_set(io, fd, events);

  return Undefined();
}


static Handle<Value> IOStart(const Arguments& args) {
  HandleScope scope;
  ev_io *io = JSToAddress<ev_io>(args[0]);
  ev_io_start(EV_DEFAULT_UC_ io);
  return Undefined();
}


static Handle<Value> IOStop(const Arguments& args) {
  HandleScope scope;
  ev_io *io = JSToAddress<ev_io>(args[0]);
  ev_io_stop(EV_DEFAULT_UC_ io);
  return Undefined();
}


void IOWatcher::Initialize(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(IOWatcher::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("IOWatcher"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "start", IOWatcher::Start);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "stop", IOWatcher::Stop);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "set", IOWatcher::Set);

  target->Set(String::NewSymbol("IOWatcher"), constructor_template->GetFunction());

  // New style IOWatchers don't use an ObjectWrap, but rather return a raw
  // pointer in javascript to the ev_io.
  NODE_SET_METHOD(target, "ioAlloc", IOAlloc);
  NODE_SET_METHOD(target, "ioSet", IOSet);
  NODE_SET_METHOD(target, "ioStop", IOStop);
  NODE_SET_METHOD(target, "ioStart", IOStart);
  NODE_SET_METHOD(target, "ioFree", IOFree);

  callback_symbol = NODE_PSYMBOL("callback");
  on_io_symbol = NODE_PSYMBOL("_onIO");
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



}  // namespace node
