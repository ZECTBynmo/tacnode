#include <node.h>
#include <node_timer.h>
#include <assert.h>

using namespace v8;
using namespace node;

static Persistent<String> repeat_symbol;


void Timer::Initialize (Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = BuildTemplate<Timer>("Timer");

  repeat_symbol = NODE_PSYMBOL("repeat");

  NODE_SET_PROTOTYPE_METHOD(t, "start", Timer::Start);
  NODE_SET_PROTOTYPE_METHOD(t, "stop", Timer::Stop);
  NODE_SET_PROTOTYPE_METHOD(t, "again", Timer::Again);

  t->InstanceTemplate()->SetAccessor(repeat_symbol,
                                     RepeatGetter,
                                     RepeatSetter);

  target->Set(String::NewSymbol("Timer"), t->GetFunction());
}


Handle<Value> Timer::RepeatGetter(Local<String> property,
                                  const AccessorInfo& info) {
  HandleScope scope;
  Timer *timer = ObjectWrap::Unwrap<Timer>(info.This());

  assert(timer);
  assert (property == repeat_symbol);

  Local<Integer> v = Integer::New(timer->watcher_.repeat);

  return scope.Close(v);
}


void Timer::RepeatSetter(Local<String> property,
                         Local<Value> value,
                         const AccessorInfo& info) {
  HandleScope scope;
  Timer *timer = ObjectWrap::Unwrap<Timer>(info.This());

  assert(timer);
  assert(property == repeat_symbol);

  timer->watcher_.repeat = NODE_V8_UNIXTIME(value);
}


void Timer::Callback(EV_P_ ev_timer *watcher, int revents) {
  Timer *timer = static_cast<Timer*>(watcher->data);

  assert(revents == EV_TIMEOUT);

  HandleScope scope;

  timer->MakeCallback(0, NULL);

  if (timer->watcher_.repeat == 0) { 
    timer->Inactive();
  }
}


Handle<Value> Timer::Start(const Arguments& args) {
  HandleScope scope;
  Timer *timer = ObjectWrap::Unwrap<Timer>(args.Holder());

  if (args.Length() != 2) {
    return ThrowException(String::New("Bad arguments"));
  }

  bool was_active = ev_is_active(&timer->watcher_);

  ev_tstamp after = NODE_V8_UNIXTIME(args[0]);
  ev_tstamp repeat = NODE_V8_UNIXTIME(args[1]);
  ev_timer_init(&timer->watcher_, Timer::Callback, after, repeat);
  timer->watcher_.data = timer;

  // Update the event loop time. Need to call this because processing JS can
  // take non-negligible amounts of time.
  ev_now_update(EV_DEFAULT_UC);

  ev_timer_start(EV_DEFAULT_UC_ &timer->watcher_);

  if (!was_active) timer->Active();

  return Undefined();
}


Handle<Value> Timer::Stop(const Arguments& args) {
  HandleScope scope;
  Timer *timer = ObjectWrap::Unwrap<Timer>(args.Holder());
  timer->Stop();
  return Undefined();
}


void Timer::Stop () {
  if (watcher_.active) {
    ev_timer_stop(EV_DEFAULT_UC_ &watcher_);
    Inactive();
  }
}


Handle<Value> Timer::Again(const Arguments& args) {
  HandleScope scope;
  Timer *timer = ObjectWrap::Unwrap<Timer>(args.Holder());

  int was_active = ev_is_active(&timer->watcher_);

  if (args.Length() > 0) {
    ev_tstamp repeat = NODE_V8_UNIXTIME(args[0]);
    if (repeat > 0) timer->watcher_.repeat = repeat;
  }

  ev_timer_again(EV_DEFAULT_UC_ &timer->watcher_);

  // ev_timer_again can start or stop the watcher.
  // So we need to check what happened and adjust the ref count
  // appropriately.

  if (ev_is_active(&timer->watcher_)) {
    if (!was_active) timer->Active();
  } else {
    if (was_active) timer->Inactive();
  }

  return Undefined();
}
