// Copyright 2009 Ryan Dahl <ry@tinyclouds.org>
#ifndef SRC_FILE_H_
#define SRC_FILE_H_

#include <node.h>
#include <node_events.h>
#include <v8.h>
#include <eio.h>

namespace node {

class File {
 public:
  static void Initialize(v8::Handle<v8::Object> target);
};

void InitFs(v8::Handle<v8::Object> target);

class EIORequest : public EventSource {
 public:
  static void Initialize(v8::Handle<v8::Object> target);

  static EIORequest* New();

  EIORequest() : EventSource() { }
  ~EIORequest() { }

  void Set(eio_req *req, v8::Handle<v8::Value> callback);

  static int After(eio_req *req);

  void Active() {
    EventSource::Active();
    ev_ref(EV_DEFAULT_UC);
  }

  void Inactive() {
    ev_unref(EV_DEFAULT_UC);
    req_ = NULL;
    EventSource::Inactive();
  }

 private:
  eio_req *req_;
};


}  // namespace node
#endif  // SRC_FILE_H_
