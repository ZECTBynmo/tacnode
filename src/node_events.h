// Copyright 2009 Ryan Dahl <ry@tinyclouds.org>
#ifndef SRC_EVENTS_H_
#define SRC_EVENTS_H_

#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>

namespace node {

// Legacy interface, do no use.
class EventEmitter : public ObjectWrap {
 public:
  static void Initialize(v8::Local<v8::FunctionTemplate> ctemplate);
  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  bool Emit(v8::Handle<v8::String> event,
            int argc,
            v8::Handle<v8::Value> argv[]);

 protected:
  EventEmitter() : ObjectWrap () { }
};


// "hard emitter" base class
class EventSource : public ObjectWrap {
 public:
  // All subclasses must call this function to initialize their JavaScript
  // counterparts. Subclasses are free to decorate the template with additional
  // and such.
  template <class T>
  static v8::Local<v8::FunctionTemplate> BuildTemplate(const char *name) {
    // TODO node_events_impl.h
    v8::HandleScope scope;

    v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(EventSource::JSNew<T>);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    t->SetClassName(v8::String::NewSymbol(name));

    return scope.Close(t);
  }

  // Subclasses to call this when they go from inactive to active.
  void Active();

  // And this when going from active to inactive
  void Inactive();

  EventSource() : ObjectWrap() { }

  virtual ~EventSource() {
    ClearStack();
    DeleteParent();
  }

  // Subclasses call this when they get a event.
  v8::Local<v8::Value> MakeCallback(int argc, v8::Handle<v8::Value> argv[]);

 private:
  template <class T>
  static v8::Handle<v8::Value> JSNew(const v8::Arguments& args) {
    // TODO node_events_impl.h
    v8::HandleScope scope;
    T *t = new T();
    t->Wrap(args.This());
    return args.This();
  }

  static void WeakParent(v8::Persistent<v8::Value> object, void* data);

  void RecordStack();
  void ClearStack();
  void DeleteParent();

  void PrintStack(int count = 0);

  v8::Persistent<v8::Object> parent_source_;
  v8::Persistent<v8::StackTrace> trace_;

  static const int kFrameLimit = 10;
  static const int kAncestorStackLimit = 10;
  static EventSource* current_source;
};


}  // namespace node
#endif  // SRC_EVENTS_H_
