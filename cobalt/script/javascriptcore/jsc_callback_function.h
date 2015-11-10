// This file was GENERATED by command:
//     pump.py jsc_callback_function.h.pump
// DO NOT EDIT BY HAND!!!


/*
 * Copyright 2015 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SCRIPT_JAVASCRIPTCORE_JSC_CALLBACK_FUNCTION_H_
#define SCRIPT_JAVASCRIPTCORE_JSC_CALLBACK_FUNCTION_H_

#include "base/logging.h"
#include "cobalt/script/callback_function.h"
#include "cobalt/script/javascriptcore/jsc_global_object.h"
#include "cobalt/script/javascriptcore/util/exception_helpers.h"
#include "cobalt/script/script_object.h"
#include "third_party/WebKit/Source/JavaScriptCore/runtime/JSFunction.h"

// The JSCCallbackFunction type is used to represent IDL callback functions.
// Create a new JSCCallbackFunction by specifying the base CallbackFunction
// as a template parameter to the constructor.
//
// Constructor parameters:
//     callable: A handle that keeps keeps alive the JSC::JSFunction that
//         will be called when the callback is fired.
//     callback: A base::Callback that will be executed when the Run(...)
//         function is executed. It will take as parameters a JSC::JSFunction
//         followed by any arguments that are defined on the callback type.

namespace cobalt {
namespace script {
namespace javascriptcore {

// First, we forward declare the Callback class template. This informs the
// compiler that the template only has 1 type parameter which is the base
// CallbackFunction template class with parameters.
//
// See base/callback.h.pump for further discussion on this pattern.
template <typename Sig>
class JSCCallbackFunction;

template <typename R>
class JSCCallbackFunction<R(void)>
    : public CallbackFunction<R(void)> {
 public:
  explicit JSCCallbackFunction(JSC::JSFunction* callable)
      : callable_(callable) { DCHECK(callable_); }

  R Run()
      const OVERRIDE {
    DCHECK(callable_);
    JSCGlobalObject* global_object =
        JSC::jsCast<JSCGlobalObject*>(callable_->globalObject());
    JSC::JSLockHolder lock(global_object->globalData());

    // http://www.w3.org/TR/WebIDL/#es-invoking-callback-functions
    // Callback 'this' is set to null, unless overridden by other specifications
    JSC::JSValue this_value = JSC::jsNull();
    JSC::MarkedArgumentBuffer args;

    JSC::CallData call_data;
    JSC::CallType call_type =
        JSC::JSFunction::getCallData(callable_, call_data);
    JSC::ExecState* exec_state = global_object->globalExec();
    JSC::JSGlobalData& global_data = global_object->globalData();
    JSC::JSValue retval =
        JSC::call(exec_state, callable_, call_type, call_data, this_value,
            args);
    if (exec_state->hadException()) {
      DLOG(WARNING) << "Exception in callback: "
                    << util::GetExceptionString(exec_state);

      exec_state->clearException();
    }
  }

  JSC::JSFunction* callable() const { return callable_; }

 private:
  JSC::JSFunction* callable_;
};

template <typename R, typename A1>
class JSCCallbackFunction<R(A1)>
    : public CallbackFunction<R(A1)> {
 public:
  explicit JSCCallbackFunction(JSC::JSFunction* callable)
      : callable_(callable) { DCHECK(callable_); }

  R Run(
      typename base::internal::CallbackParamTraits<A1>::ForwardType a1)
      const OVERRIDE {
    DCHECK(callable_);
    JSCGlobalObject* global_object =
        JSC::jsCast<JSCGlobalObject*>(callable_->globalObject());
    JSC::JSLockHolder lock(global_object->globalData());

    // http://www.w3.org/TR/WebIDL/#es-invoking-callback-functions
    // Callback 'this' is set to null, unless overridden by other specifications
    JSC::JSValue this_value = JSC::jsNull();
    JSC::MarkedArgumentBuffer args;
    args.append(ToJSValue(global_object, a1));

    JSC::CallData call_data;
    JSC::CallType call_type =
        JSC::JSFunction::getCallData(callable_, call_data);
    JSC::ExecState* exec_state = global_object->globalExec();
    JSC::JSGlobalData& global_data = global_object->globalData();
    JSC::JSValue retval =
        JSC::call(exec_state, callable_, call_type, call_data, this_value,
            args);
    if (exec_state->hadException()) {
      DLOG(WARNING) << "Exception in callback: "
                    << util::GetExceptionString(exec_state);

      exec_state->clearException();
    }
  }

  JSC::JSFunction* callable() const { return callable_; }

 private:
  JSC::JSFunction* callable_;
};

template <typename R, typename A1, typename A2>
class JSCCallbackFunction<R(A1, A2)>
    : public CallbackFunction<R(A1, A2)> {
 public:
  explicit JSCCallbackFunction(JSC::JSFunction* callable)
      : callable_(callable) { DCHECK(callable_); }

  R Run(
      typename base::internal::CallbackParamTraits<A1>::ForwardType a1,
      typename base::internal::CallbackParamTraits<A2>::ForwardType a2)
      const OVERRIDE {
    DCHECK(callable_);
    JSCGlobalObject* global_object =
        JSC::jsCast<JSCGlobalObject*>(callable_->globalObject());
    JSC::JSLockHolder lock(global_object->globalData());

    // http://www.w3.org/TR/WebIDL/#es-invoking-callback-functions
    // Callback 'this' is set to null, unless overridden by other specifications
    JSC::JSValue this_value = JSC::jsNull();
    JSC::MarkedArgumentBuffer args;
    args.append(ToJSValue(global_object, a1));
    args.append(ToJSValue(global_object, a2));

    JSC::CallData call_data;
    JSC::CallType call_type =
        JSC::JSFunction::getCallData(callable_, call_data);
    JSC::ExecState* exec_state = global_object->globalExec();
    JSC::JSGlobalData& global_data = global_object->globalData();
    JSC::JSValue retval =
        JSC::call(exec_state, callable_, call_type, call_data, this_value,
            args);
    if (exec_state->hadException()) {
      DLOG(WARNING) << "Exception in callback: "
                    << util::GetExceptionString(exec_state);

      exec_state->clearException();
    }
  }

  JSC::JSFunction* callable() const { return callable_; }

 private:
  JSC::JSFunction* callable_;
};

template <typename R, typename A1, typename A2, typename A3>
class JSCCallbackFunction<R(A1, A2, A3)>
    : public CallbackFunction<R(A1, A2, A3)> {
 public:
  explicit JSCCallbackFunction(JSC::JSFunction* callable)
      : callable_(callable) { DCHECK(callable_); }

  R Run(
      typename base::internal::CallbackParamTraits<A1>::ForwardType a1,
      typename base::internal::CallbackParamTraits<A2>::ForwardType a2,
      typename base::internal::CallbackParamTraits<A3>::ForwardType a3)
      const OVERRIDE {
    DCHECK(callable_);
    JSCGlobalObject* global_object =
        JSC::jsCast<JSCGlobalObject*>(callable_->globalObject());
    JSC::JSLockHolder lock(global_object->globalData());

    // http://www.w3.org/TR/WebIDL/#es-invoking-callback-functions
    // Callback 'this' is set to null, unless overridden by other specifications
    JSC::JSValue this_value = JSC::jsNull();
    JSC::MarkedArgumentBuffer args;
    args.append(ToJSValue(global_object, a1));
    args.append(ToJSValue(global_object, a2));
    args.append(ToJSValue(global_object, a3));

    JSC::CallData call_data;
    JSC::CallType call_type =
        JSC::JSFunction::getCallData(callable_, call_data);
    JSC::ExecState* exec_state = global_object->globalExec();
    JSC::JSGlobalData& global_data = global_object->globalData();
    JSC::JSValue retval =
        JSC::call(exec_state, callable_, call_type, call_data, this_value,
            args);
    if (exec_state->hadException()) {
      DLOG(WARNING) << "Exception in callback: "
                    << util::GetExceptionString(exec_state);

      exec_state->clearException();
    }
  }

  JSC::JSFunction* callable() const { return callable_; }

 private:
  JSC::JSFunction* callable_;
};

template <typename R, typename A1, typename A2, typename A3, typename A4>
class JSCCallbackFunction<R(A1, A2, A3, A4)>
    : public CallbackFunction<R(A1, A2, A3, A4)> {
 public:
  explicit JSCCallbackFunction(JSC::JSFunction* callable)
      : callable_(callable) { DCHECK(callable_); }

  R Run(
      typename base::internal::CallbackParamTraits<A1>::ForwardType a1,
      typename base::internal::CallbackParamTraits<A2>::ForwardType a2,
      typename base::internal::CallbackParamTraits<A3>::ForwardType a3,
      typename base::internal::CallbackParamTraits<A4>::ForwardType a4)
      const OVERRIDE {
    DCHECK(callable_);
    JSCGlobalObject* global_object =
        JSC::jsCast<JSCGlobalObject*>(callable_->globalObject());
    JSC::JSLockHolder lock(global_object->globalData());

    // http://www.w3.org/TR/WebIDL/#es-invoking-callback-functions
    // Callback 'this' is set to null, unless overridden by other specifications
    JSC::JSValue this_value = JSC::jsNull();
    JSC::MarkedArgumentBuffer args;
    args.append(ToJSValue(global_object, a1));
    args.append(ToJSValue(global_object, a2));
    args.append(ToJSValue(global_object, a3));
    args.append(ToJSValue(global_object, a4));

    JSC::CallData call_data;
    JSC::CallType call_type =
        JSC::JSFunction::getCallData(callable_, call_data);
    JSC::ExecState* exec_state = global_object->globalExec();
    JSC::JSGlobalData& global_data = global_object->globalData();
    JSC::JSValue retval =
        JSC::call(exec_state, callable_, call_type, call_data, this_value,
            args);
    if (exec_state->hadException()) {
      DLOG(WARNING) << "Exception in callback: "
                    << util::GetExceptionString(exec_state);

      exec_state->clearException();
    }
  }

  JSC::JSFunction* callable() const { return callable_; }

 private:
  JSC::JSFunction* callable_;
};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5>
class JSCCallbackFunction<R(A1, A2, A3, A4, A5)>
    : public CallbackFunction<R(A1, A2, A3, A4, A5)> {
 public:
  explicit JSCCallbackFunction(JSC::JSFunction* callable)
      : callable_(callable) { DCHECK(callable_); }

  R Run(
      typename base::internal::CallbackParamTraits<A1>::ForwardType a1,
      typename base::internal::CallbackParamTraits<A2>::ForwardType a2,
      typename base::internal::CallbackParamTraits<A3>::ForwardType a3,
      typename base::internal::CallbackParamTraits<A4>::ForwardType a4,
      typename base::internal::CallbackParamTraits<A5>::ForwardType a5)
      const OVERRIDE {
    DCHECK(callable_);
    JSCGlobalObject* global_object =
        JSC::jsCast<JSCGlobalObject*>(callable_->globalObject());
    JSC::JSLockHolder lock(global_object->globalData());

    // http://www.w3.org/TR/WebIDL/#es-invoking-callback-functions
    // Callback 'this' is set to null, unless overridden by other specifications
    JSC::JSValue this_value = JSC::jsNull();
    JSC::MarkedArgumentBuffer args;
    args.append(ToJSValue(global_object, a1));
    args.append(ToJSValue(global_object, a2));
    args.append(ToJSValue(global_object, a3));
    args.append(ToJSValue(global_object, a4));
    args.append(ToJSValue(global_object, a5));

    JSC::CallData call_data;
    JSC::CallType call_type =
        JSC::JSFunction::getCallData(callable_, call_data);
    JSC::ExecState* exec_state = global_object->globalExec();
    JSC::JSGlobalData& global_data = global_object->globalData();
    JSC::JSValue retval =
        JSC::call(exec_state, callable_, call_type, call_data, this_value,
            args);
    if (exec_state->hadException()) {
      DLOG(WARNING) << "Exception in callback: "
                    << util::GetExceptionString(exec_state);

      exec_state->clearException();
    }
  }

  JSC::JSFunction* callable() const { return callable_; }

 private:
  JSC::JSFunction* callable_;
};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6>
class JSCCallbackFunction<R(A1, A2, A3, A4, A5, A6)>
    : public CallbackFunction<R(A1, A2, A3, A4, A5, A6)> {
 public:
  explicit JSCCallbackFunction(JSC::JSFunction* callable)
      : callable_(callable) { DCHECK(callable_); }

  R Run(
      typename base::internal::CallbackParamTraits<A1>::ForwardType a1,
      typename base::internal::CallbackParamTraits<A2>::ForwardType a2,
      typename base::internal::CallbackParamTraits<A3>::ForwardType a3,
      typename base::internal::CallbackParamTraits<A4>::ForwardType a4,
      typename base::internal::CallbackParamTraits<A5>::ForwardType a5,
      typename base::internal::CallbackParamTraits<A6>::ForwardType a6)
      const OVERRIDE {
    DCHECK(callable_);
    JSCGlobalObject* global_object =
        JSC::jsCast<JSCGlobalObject*>(callable_->globalObject());
    JSC::JSLockHolder lock(global_object->globalData());

    // http://www.w3.org/TR/WebIDL/#es-invoking-callback-functions
    // Callback 'this' is set to null, unless overridden by other specifications
    JSC::JSValue this_value = JSC::jsNull();
    JSC::MarkedArgumentBuffer args;
    args.append(ToJSValue(global_object, a1));
    args.append(ToJSValue(global_object, a2));
    args.append(ToJSValue(global_object, a3));
    args.append(ToJSValue(global_object, a4));
    args.append(ToJSValue(global_object, a5));
    args.append(ToJSValue(global_object, a6));

    JSC::CallData call_data;
    JSC::CallType call_type =
        JSC::JSFunction::getCallData(callable_, call_data);
    JSC::ExecState* exec_state = global_object->globalExec();
    JSC::JSGlobalData& global_data = global_object->globalData();
    JSC::JSValue retval =
        JSC::call(exec_state, callable_, call_type, call_data, this_value,
            args);
    if (exec_state->hadException()) {
      DLOG(WARNING) << "Exception in callback: "
                    << util::GetExceptionString(exec_state);

      exec_state->clearException();
    }
  }

  JSC::JSFunction* callable() const { return callable_; }

 private:
  JSC::JSFunction* callable_;
};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6, typename A7>
class JSCCallbackFunction<R(A1, A2, A3, A4, A5, A6, A7)>
    : public CallbackFunction<R(A1, A2, A3, A4, A5, A6, A7)> {
 public:
  explicit JSCCallbackFunction(JSC::JSFunction* callable)
      : callable_(callable) { DCHECK(callable_); }

  R Run(
      typename base::internal::CallbackParamTraits<A1>::ForwardType a1,
      typename base::internal::CallbackParamTraits<A2>::ForwardType a2,
      typename base::internal::CallbackParamTraits<A3>::ForwardType a3,
      typename base::internal::CallbackParamTraits<A4>::ForwardType a4,
      typename base::internal::CallbackParamTraits<A5>::ForwardType a5,
      typename base::internal::CallbackParamTraits<A6>::ForwardType a6,
      typename base::internal::CallbackParamTraits<A7>::ForwardType a7)
      const OVERRIDE {
    DCHECK(callable_);
    JSCGlobalObject* global_object =
        JSC::jsCast<JSCGlobalObject*>(callable_->globalObject());
    JSC::JSLockHolder lock(global_object->globalData());

    // http://www.w3.org/TR/WebIDL/#es-invoking-callback-functions
    // Callback 'this' is set to null, unless overridden by other specifications
    JSC::JSValue this_value = JSC::jsNull();
    JSC::MarkedArgumentBuffer args;
    args.append(ToJSValue(global_object, a1));
    args.append(ToJSValue(global_object, a2));
    args.append(ToJSValue(global_object, a3));
    args.append(ToJSValue(global_object, a4));
    args.append(ToJSValue(global_object, a5));
    args.append(ToJSValue(global_object, a6));
    args.append(ToJSValue(global_object, a7));

    JSC::CallData call_data;
    JSC::CallType call_type =
        JSC::JSFunction::getCallData(callable_, call_data);
    JSC::ExecState* exec_state = global_object->globalExec();
    JSC::JSGlobalData& global_data = global_object->globalData();
    JSC::JSValue retval =
        JSC::call(exec_state, callable_, call_type, call_data, this_value,
            args);
    if (exec_state->hadException()) {
      DLOG(WARNING) << "Exception in callback: "
                    << util::GetExceptionString(exec_state);

      exec_state->clearException();
    }
  }

  JSC::JSFunction* callable() const { return callable_; }

 private:
  JSC::JSFunction* callable_;
};

}  // namespace javascriptcore
}  // namespace script
}  // namespace cobalt

#endif  // SCRIPT_JAVASCRIPTCORE_JSC_CALLBACK_FUNCTION_H_
