#ifndef node_timer_h
#define node_timer_h

#include <node.h>
#include <node_events.h>
#include <v8.h>
#include <ev.h>

namespace node {

class Timer : public EventSource {
 public:
  static void Initialize (v8::Handle<v8::Object> target);

  Timer () : EventSource () {
    // dummy timeout values
    ev_timer_init(&watcher_, Callback, 0., 1.);
    watcher_.data = this;
  }

 protected:

  ~Timer() {
    ev_timer_stop(EV_DEFAULT_UC_ &watcher_);
  }

  static v8::Handle<v8::Value> Start (const v8::Arguments& args);
  static v8::Handle<v8::Value> Stop (const v8::Arguments& args);
  static v8::Handle<v8::Value> Again (const v8::Arguments& args);
  static v8::Handle<v8::Value> RepeatGetter (v8::Local<v8::String> property, const v8::AccessorInfo& info);
  static void RepeatSetter (v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info);

 private:
  static void Callback (EV_P_ ev_timer *watcher, int revents);
  void Stop ();
  ev_timer watcher_;
};

} // namespace node
#endif //  node_timer_h
