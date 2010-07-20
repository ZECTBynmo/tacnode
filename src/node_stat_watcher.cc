// Copyright 2009 Ryan Dahl <ry@tinyclouds.org>
#include <node_stat_watcher.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

namespace node {

using namespace v8;

void StatWatcher::Initialize(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = BuildTemplate<StatWatcher>("StatWatcher");

  NODE_SET_PROTOTYPE_METHOD(t, "start", Start);
  NODE_SET_PROTOTYPE_METHOD(t, "stop", Stop);

  target->Set(String::NewSymbol("StatWatcher"), t->GetFunction());
}


void StatWatcher::Callback(EV_P_ ev_stat *watcher, int revents) {
  assert(revents == EV_STAT);
  StatWatcher *w = static_cast<StatWatcher*>(watcher->data);
  assert(watcher == &w->watcher_);

  HandleScope scope;
  Handle<Value> argv[2];
  argv[0] = Handle<Value>(BuildStatsObject(&watcher->attr));
  argv[1] = Handle<Value>(BuildStatsObject(&watcher->prev));
  w->MakeCallback(2, argv);
}


Handle<Value> StatWatcher::Start(const Arguments& args) {
  HandleScope scope;

  if (args.Length() < 1 || !args[0]->IsString()) {
    return ThrowException(Exception::TypeError(String::New("Bad arguments")));
  }

  StatWatcher *w = ObjectWrap::Unwrap<StatWatcher>(args.Holder());

  String::Utf8Value path(args[0]->ToString());
  assert(w->path_ == NULL);
  w->path_ = strdup(*path);

  w->persistent_ = args[1]->IsTrue();

  ev_tstamp interval = 0.;
  if (args[2]->IsInt32()) {
    interval = NODE_V8_UNIXTIME(args[2]);
  }

  ev_stat_set(&w->watcher_, w->path_, interval);
  ev_stat_start(EV_DEFAULT_UC_ &w->watcher_);

  w->Active();
  if (!w->persistent_) ev_unref(EV_DEFAULT_UC);

  return Undefined();
}


Handle<Value> StatWatcher::Stop(const Arguments& args) {
  HandleScope scope;
  StatWatcher *w = ObjectWrap::Unwrap<StatWatcher>(args.Holder());
  w->Stop();
  return Undefined();
}


void StatWatcher::Stop () {
  if (watcher_.active) {
    if (!persistent_) ev_ref(EV_DEFAULT_UC);
    ev_stat_stop(EV_DEFAULT_UC_ &watcher_);
    Inactive();
    free(path_);
    path_ = NULL;
  }
}


}  // namespace node
