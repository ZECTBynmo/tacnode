// Copyright 2009 Ryan Dahl <ry@tinyclouds.org>
#include <node_events.h>
#include <node.h>

namespace node {

using namespace v8;

Persistent<FunctionTemplate> EventEmitter::constructor_template;

static Persistent<String> events_symbol;

void EventEmitter::Initialize(Local<FunctionTemplate> ctemplate) {
  HandleScope scope;

  constructor_template = Persistent<FunctionTemplate>::New(ctemplate);

  constructor_template->SetClassName(String::NewSymbol("EventEmitter"));

  events_symbol = NODE_PSYMBOL("_events");

  // All other prototype methods are defined in events.js
}


bool EventEmitter::Emit(Handle<String> event, int argc, Handle<Value> argv[]) {
  HandleScope scope;
  // HandleScope not needed here because only called from one of the two
  // functions below
  Local<Value> events_v = handle_->Get(events_symbol);
  if (!events_v->IsObject()) return false;
  Local<Object> events = events_v->ToObject();

  Local<Value> listeners_v = events->Get(event);

  TryCatch try_catch;

  if (listeners_v->IsFunction()) {
    // Optimized one-listener case
    Local<Function> listener = Local<Function>::Cast(listeners_v);

    listener->Call(handle_, argc, argv);

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
      return false;
    }

  } else if (listeners_v->IsArray()) {
    Local<Array> listeners = Local<Array>::Cast(listeners_v->ToObject()->Clone());

    for (uint32_t i = 0; i < listeners->Length(); i++) {
      Local<Value> listener_v = listeners->Get(i);
      if (!listener_v->IsFunction()) continue;
      Local<Function> listener = Local<Function>::Cast(listener_v);

      listener->Call(handle_, argc, argv);

      if (try_catch.HasCaught()) {
        FatalException(try_catch);
        return false;
      }
    }

  } else {
    return false;
  }

  return true;
}


// EventSource

EventSource* EventSource::current_source;


Local<Value> EventSource::MakeCallback(int argc, Handle<Value> argv[]) {
  HandleScope scope;

  Local<Value> callback_v = handle_->Get(String::NewSymbol("callback"));
  if (!callback_v->IsFunction()) return Local<Value>();
  Local<Function> callback = Local<Function>::Cast(callback_v);

  // TODO DTrace probe here.

  TryCatch try_catch;

  assert(current_source == NULL);
  current_source = this;

  Local<Value> ret = callback->Call(handle_, argc, argv);

  current_source = NULL;

  if (try_catch.HasCaught()) {
    // TODO Look for 'uncaughtException' handlers. 

    // Here we print the stack trace from try_catch
    ReportException(try_catch, true);
    // Then we print the stored stacktrace plus our ancestors stacks.
    PrintStack();
    // XXX Stop whatever activity might have been going on?

    exit(1);
  }

  // Call nextTick callbacks?

  return scope.Close(ret);
}


void EventSource::PrintStack(int count) {
  if (trace_.IsEmpty()) return;


  // Print from for this EventSource
  fprintf(stderr, "    ---------------------------\n");
  int length = trace_->GetFrameCount();
  for (int i = 0; i < length; i++) {
    Local<StackFrame> frame = trace_->GetFrame(i);

    String::Utf8Value script_name(frame->GetScriptName());
    String::Utf8Value function_name(frame->GetFunctionName());
    int column = frame->GetColumn();
    int line_number = frame->GetLineNumber();

    // how do you cast Value to StackFrame ?
    // print frame somehow...
    fprintf(stderr, 
            "    at %s (%s:%d:%d)\n",
            *function_name,
            *script_name,
            line_number,
            column);
  }

  // Recursively print up to kAncestorStackLimit ancestor traces...
  if (parent_source_.IsEmpty() == false && count < kAncestorStackLimit) {
    EventSource *parent = ObjectWrap::Unwrap<EventSource>(parent_source_);
    parent->PrintStack(count + 1);
  }
}


void EventSource::Active() {
  Ref();
  RecordStack();
  // TODO DTrace probe here.
}


void EventSource::Inactive() {
  DeleteParent();
  ClearStack();
  Unref();
}


void EventSource::ClearStack() {
  if (!trace_.IsEmpty()) {
    trace_.Dispose();
    trace_.Clear();
  }
}


void EventSource::RecordStack() {
  HandleScope scope;

  ClearStack();

  // Assume inside HandleScope
  Local<StackTrace> trace =
      StackTrace::CurrentStackTrace(kFrameLimit, StackTrace::kOverview);

  trace_ = Persistent<StackTrace>::New(trace);

  // Set parent.
  if (current_source) {
    parent_source_ = Persistent<Object>::New(current_source->handle_);
    parent_source_.MakeWeak(this, WeakParent);
  }
}


void EventSource::WeakParent(Persistent<Value> object, void* data) {
  EventSource *s = static_cast<EventSource*>(data);
  assert(s->parent_source_->StrictEquals(object));
  s->DeleteParent();
}


void EventSource::DeleteParent() {
  if (!parent_source_.IsEmpty()) {
    parent_source_.ClearWeak();
    parent_source_.Dispose();
    parent_source_.Clear();
  }
}


}  // namespace node
