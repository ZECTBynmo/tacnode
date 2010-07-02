// Copyright 2009 Ryan Dahl <ry@tinyclouds.org>
#ifndef NODE_IO_H_
#define NODE_IO_H_

#include <node_events.h>
#include <ev.h>

namespace node {

class IOWatcher : public EventSource {
 public:
  static void Initialize(v8::Handle<v8::Object> target);

  IOWatcher() : EventSource() {
    ev_init(&watcher_, IOWatcher::Callback);
    watcher_.data = this;
  }

 protected:
  ~IOWatcher() {
    assert(!ev_is_active(&watcher_));
    assert(!ev_is_pending(&watcher_));
  }

  static v8::Handle<v8::Value> Set(const v8::Arguments& args);
  static v8::Handle<v8::Value> Start(const v8::Arguments& args);
  static v8::Handle<v8::Value> Stop(const v8::Arguments& args);

 private:
  static void Callback(EV_P_ ev_io *watcher, int revents);

  ev_io watcher_;
};

}  // namespace node
#endif  // NODE_IO_H_

