// Copyright 2009 Ryan Dahl <ry@tinyclouds.org>
#include <node_io_watcher.h>

#include <node.h>
#include <node_events.h>
#include <v8.h>

#include <assert.h>

namespace node {

using namespace v8;


void IOWatcher::Initialize(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = BuildTemplate<IOWatcher>("IOWatcher");

  NODE_SET_PROTOTYPE_METHOD(t, "set", IOWatcher::Set);
  NODE_SET_PROTOTYPE_METHOD(t, "start", IOWatcher::Start);
  NODE_SET_PROTOTYPE_METHOD(t, "stop", IOWatcher::Stop);

  target->Set(String::NewSymbol("IOWatcher"), t->GetFunction());
}


void IOWatcher::Callback(EV_P_ ev_io *w, int revents) {
  IOWatcher *io = static_cast<IOWatcher*>(w->data);
  assert(w == &io->watcher_);
  HandleScope scope;

  Local<Value> argv[2];
  argv[0] = Local<Value>::New(revents & EV_READ ? True() : False());
  argv[1] = Local<Value>::New(revents & EV_WRITE ? True() : False());

  io->MakeCallback(2, argv);
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

  ev_io_set(&io->watcher_, fd, events);

  return Undefined();
}


Handle<Value> IOWatcher::Start(const Arguments& args) {
  HandleScope scope;
  IOWatcher *io = ObjectWrap::Unwrap<IOWatcher>(args.Holder());

  if (!ev_is_active(&io->watcher_)) {
    ev_io_start(EV_DEFAULT_UC_ &io->watcher_);
    io->Active();
  }

  return Undefined();
}


Handle<Value> IOWatcher::Stop(const Arguments& args) {
  HandleScope scope;
  IOWatcher *io = ObjectWrap::Unwrap<IOWatcher>(args.Holder());

  if (ev_is_active(&io->watcher_)) {
    ev_io_stop(EV_DEFAULT_UC_ &io->watcher_);
    io->Inactive();
  }

  return Undefined();
}


}  // namespace node
