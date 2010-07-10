// Copyright 2009 Ryan Dahl <ry@tinyclouds.org>
#include <node_signal_watcher.h>
#include <assert.h>

namespace node {

using namespace v8;


void SignalWatcher::Initialize(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = BuildTemplate<SignalWatcher>("SignalWatcher");

  NODE_SET_PROTOTYPE_METHOD(t, "start", Start);
  NODE_SET_PROTOTYPE_METHOD(t, "stop", Stop);
  NODE_SET_PROTOTYPE_METHOD(t, "set", Set);

  target->Set(String::NewSymbol("SignalWatcher"), t->GetFunction());
}


void SignalWatcher::Callback(EV_P_ ev_signal *watcher, int revents) {
  SignalWatcher *w = static_cast<SignalWatcher*>(watcher->data);
  assert(watcher == &w->watcher_);
  w->MakeCallback(0, NULL);
}


Handle<Value> SignalWatcher::Set(const Arguments& args) {
  HandleScope scope;
  SignalWatcher *w = ObjectWrap::Unwrap<SignalWatcher>(args.Holder());

  if (args.Length() != 1 || !args[0]->IsInt32()) {
    return ThrowException(String::New("Bad arguments"));
  }

  int sig = args[0]->Int32Value();

  ev_signal_set(&w->watcher_, sig);
  return Undefined();
}


Handle<Value> SignalWatcher::Start(const Arguments& args) {
  HandleScope scope;
  SignalWatcher *w = ObjectWrap::Unwrap<SignalWatcher>(args.Holder());
  w->Start();
  return Undefined();
}


void SignalWatcher::Start () {
  if (!ev_is_active(&watcher_)) {
    ev_signal_start(EV_DEFAULT_UC_ &watcher_);
    ev_unref(EV_DEFAULT_UC);
    Active();
  }
}


Handle<Value> SignalWatcher::Stop(const Arguments& args) {
  HandleScope scope;
  SignalWatcher *w = ObjectWrap::Unwrap<SignalWatcher>(args.Holder());
  w->Stop();
  return Undefined();
}


void SignalWatcher::Stop () {
  if (ev_is_active(&watcher_)) {
    ev_ref(EV_DEFAULT_UC);
    ev_signal_stop(EV_DEFAULT_UC_ &watcher_);
    Inactive();
  }
}


}  // namespace node

NODE_MODULE(node_signal_watcher, node::SignalWatcher::Initialize);
