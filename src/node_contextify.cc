// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "node.h"
#include "node_internals.h"

namespace node {

using v8::AccessType;
using v8::Array;
using v8::Boolean;
using v8::Context;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::HandleScope;
using v8::Integer;
using v8::Local;
using v8::None;
using v8::Object;
using v8::ObjectTemplate;
using v8::Persistent;
using v8::PropertyCallbackInfo;
using v8::Script;
using v8::String;
using v8::TryCatch;
using v8::Value;


class ContextifyContext : ObjectWrap {
 private:
  Persistent<Object> sandbox_;
  Persistent<Object> proxy_global_;
  static Persistent<FunctionTemplate> data_wrapper_tmpl;
  static Persistent<Function> data_wrapper_ctor;

 public:
  Persistent<Context> context_;
  static Persistent<FunctionTemplate> js_tmpl;

  explicit ContextifyContext(Local<Object> sandbox) :
      sandbox_(node_isolate, sandbox) {
  }


  ~ContextifyContext() {
    context_.Dispose();
    proxy_global_.Dispose();
    sandbox_.Dispose();
  }


  // We override ObjectWrap::Wrap so that we can create our context after
  // we have a reference to our "host" JavaScript object.  If we try to use
  // handle_ in the ContextifyContext constructor, it will be empty since it's
  // set in ObjectWrap::Wrap.
  inline void Wrap(Local<Object> handle) {
    HandleScope scope(node_isolate);
    ObjectWrap::Wrap(handle);
    Local<Context> v8_context = CreateV8Context();
    context_.Reset(node_isolate, v8_context);
    proxy_global_.Reset(node_isolate, v8_context->Global());
  }


  // This is an object that just keeps an internal pointer to this
  // ContextifyContext.  It's passed to the NamedPropertyHandler.  If we
  // pass the main JavaScript context object we're embedded in, then the
  // NamedPropertyHandler will store a reference to it forever and keep it
  // from getting gc'd.
  Local<Value> CreateDataWrapper() {
    HandleScope scope(node_isolate);
    Local<Function> ctor = PersistentToLocal(node_isolate, data_wrapper_ctor);
    Local<Object> wrapper = ctor->NewInstance();
    NODE_WRAP(wrapper, this);
    return scope.Close(wrapper);
  }


  Local<Context> CreateV8Context() {
    HandleScope scope(node_isolate);
    Local<FunctionTemplate> function_template = FunctionTemplate::New();
    function_template->SetHiddenPrototype(true);

    Local<Object> sandbox = PersistentToLocal(node_isolate, sandbox_);
    function_template->SetClassName(sandbox->GetConstructorName());

    Local<ObjectTemplate> object_template =
        function_template->InstanceTemplate();
    object_template->SetNamedPropertyHandler(GlobalPropertyGetterCallback,
                                             GlobalPropertySetterCallback,
                                             GlobalPropertyQueryCallback,
                                             GlobalPropertyDeleterCallback,
                                             GlobalPropertyEnumeratorCallback,
                                             CreateDataWrapper());
    object_template->SetAccessCheckCallbacks(GlobalPropertyNamedAccessCheck,
                                             GlobalPropertyIndexedAccessCheck);
    return scope.Close(Context::New(node_isolate, NULL, object_template));
  }


  static void Init(Local<Object> target) {
    HandleScope scope(node_isolate);

    Local<FunctionTemplate> function_template = FunctionTemplate::New();
    function_template->InstanceTemplate()->SetInternalFieldCount(1);
    data_wrapper_tmpl.Reset(node_isolate, function_template);

    Local<FunctionTemplate> lwrapper_tmpl =
        PersistentToLocal(node_isolate, data_wrapper_tmpl);
    data_wrapper_ctor.Reset(node_isolate, lwrapper_tmpl->GetFunction());

    js_tmpl.Reset(node_isolate, FunctionTemplate::New(New));
    Local<FunctionTemplate> ljs_tmpl = PersistentToLocal(node_isolate, js_tmpl);
    ljs_tmpl->InstanceTemplate()->SetInternalFieldCount(1);

    Local<String> class_name
        = FIXED_ONE_BYTE_STRING(node_isolate, "ContextifyContext");
    ljs_tmpl->SetClassName(class_name);
    target->Set(class_name, ljs_tmpl->GetFunction());
  }


  // args[0] = the sandbox object
  static void New(const FunctionCallbackInfo<Value>& args) {
    HandleScope scope(node_isolate);
    if (!args[0]->IsObject()) {
      return ThrowTypeError("sandbox argument must be an object.");
    }
    ContextifyContext* ctx = new ContextifyContext(args[0].As<Object>());
    ctx->Wrap(args.This());
  }


  static bool InstanceOf(Local<Value> value) {
    return !value.IsEmpty() &&
        PersistentToLocal(node_isolate, js_tmpl)->HasInstance(value);
  }


  static bool GlobalPropertyNamedAccessCheck(Local<Object> host,
                                             Local<Value> key,
                                             AccessType type,
                                             Local<Value> data) {
    return true;
  }


  static bool GlobalPropertyIndexedAccessCheck(Local<Object> host,
                                               uint32_t key,
                                               AccessType type,
                                               Local<Value> data) {
    return true;
  }


  static void GlobalPropertyGetterCallback(
      Local<String> property,
      const PropertyCallbackInfo<Value>& args) {
    HandleScope scope(node_isolate);

    Local<Object> data = args.Data()->ToObject();
    ContextifyContext* ctx = ObjectWrap::Unwrap<ContextifyContext>(data);

    Local<Object> sandbox = PersistentToLocal(node_isolate, ctx->sandbox_);
    Local<Value> rv = sandbox->GetRealNamedProperty(property);
    if (rv.IsEmpty()) {
      Local<Object> proxy_global = PersistentToLocal(node_isolate,
                                                     ctx->proxy_global_);
      rv = proxy_global->GetRealNamedProperty(property);
    }

    args.GetReturnValue().Set(rv);
  }


  static void GlobalPropertySetterCallback(
      Local<String> property,
      Local<Value> value,
      const PropertyCallbackInfo<Value>& args) {
    HandleScope scope(node_isolate);

    Local<Object> data = args.Data()->ToObject();
    ContextifyContext* ctx = ObjectWrap::Unwrap<ContextifyContext>(data);

    PersistentToLocal(node_isolate, ctx->sandbox_)->Set(property, value);
  }


  static void GlobalPropertyQueryCallback(
      Local<String> property,
      const PropertyCallbackInfo<Integer>& args) {
    HandleScope scope(node_isolate);

    Local<Object> data = args.Data()->ToObject();
    ContextifyContext* ctx = ObjectWrap::Unwrap<ContextifyContext>(data);

    Local<Object> sandbox = PersistentToLocal(node_isolate, ctx->sandbox_);
    Local<Object> proxy_global = PersistentToLocal(node_isolate,
                                                   ctx->proxy_global_);

    bool in_sandbox = sandbox->GetRealNamedProperty(property).IsEmpty();
    bool in_proxy_global =
        proxy_global->GetRealNamedProperty(property).IsEmpty();
    if (!in_sandbox || !in_proxy_global) {
      args.GetReturnValue().Set(None);
    }
  }


  static void GlobalPropertyDeleterCallback(
      Local<String> property,
      const PropertyCallbackInfo<Boolean>& args) {
    HandleScope scope(node_isolate);

    Local<Object> data = args.Data()->ToObject();
    ContextifyContext* ctx = ObjectWrap::Unwrap<ContextifyContext>(data);

    bool success = PersistentToLocal(node_isolate,
                                     ctx->sandbox_)->Delete(property);
    if (!success) {
      success = PersistentToLocal(node_isolate,
                                  ctx->proxy_global_)->Delete(property);
    }
    args.GetReturnValue().Set(success);
  }


  static void GlobalPropertyEnumeratorCallback(
      const PropertyCallbackInfo<Array>& args) {
    HandleScope scope(node_isolate);

    Local<Object> data = args.Data()->ToObject();
    ContextifyContext* ctx = ObjectWrap::Unwrap<ContextifyContext>(data);

    Local<Object> sandbox = PersistentToLocal(node_isolate, ctx->sandbox_);
    args.GetReturnValue().Set(sandbox->GetPropertyNames());
  }
};

class ContextifyScript : ObjectWrap {
 private:
  Persistent<Script> script_;

 public:
  static Persistent<FunctionTemplate> script_tmpl;

  static void Init(Local<Object> target) {
    HandleScope scope(node_isolate);
    Local<String> class_name =
        FIXED_ONE_BYTE_STRING(node_isolate, "ContextifyScript");

    script_tmpl.Reset(node_isolate, FunctionTemplate::New(New));
    Local<FunctionTemplate> lscript_tmpl =
        PersistentToLocal(node_isolate, script_tmpl);
    lscript_tmpl->InstanceTemplate()->SetInternalFieldCount(1);
    lscript_tmpl->SetClassName(class_name);
    NODE_SET_PROTOTYPE_METHOD(lscript_tmpl, "runInContext", RunInContext);
    NODE_SET_PROTOTYPE_METHOD(lscript_tmpl,
                              "runInThisContext",
                              RunInThisContext);

    target->Set(class_name, lscript_tmpl->GetFunction());
  }


  static void New(const FunctionCallbackInfo<Value>& args) {
    HandleScope scope(node_isolate);

    ContextifyScript *contextify_script = new ContextifyScript();
    contextify_script->Wrap(args.Holder());
    Local<String> code = args[0]->ToString();
    Local<String> filename = !args[1]->IsUndefined()
        ? args[1]->ToString()
        : FIXED_ONE_BYTE_STRING(node_isolate, "evalmachine.<anonymous>");

    Local<Context> context = Context::GetCurrent();
    Context::Scope context_scope(context);

    TryCatch trycatch;

    Local<Script> v8_script = Script::New(code, filename);

    if (v8_script.IsEmpty()) {
      trycatch.ReThrow();
      return;
    }
    contextify_script->script_.Reset(node_isolate, v8_script);
  }


  static void RunInContext(const FunctionCallbackInfo<Value>& args) {
    HandleScope scope(node_isolate);
    if (!ContextifyContext::InstanceOf(args[0]->ToObject())) {
      return ThrowTypeError("sandbox argument must be an object.");
    }
    ContextifyContext* ctx =
        ObjectWrap::Unwrap<ContextifyContext>(args[0]->ToObject());
    Persistent<Context> context;
    context.Reset(node_isolate, ctx->context_);
    Local<Context> lcontext = PersistentToLocal(node_isolate, context);
    Context::Scope context_scope(lcontext);

    // Now that we've set up the current context to be the passed-in one, run
    // the code in the current context ("this context").
    RunInThisContext(args);
  }


  static void RunInThisContext(const FunctionCallbackInfo<Value>& args) {
    ContextifyScript* wrapped_script =
        ObjectWrap::Unwrap<ContextifyScript>(args.This());
    Local<Script> script = PersistentToLocal(node_isolate,
                                             wrapped_script->script_);
    TryCatch trycatch;
    if (script.IsEmpty()) {
      trycatch.ReThrow();
      return;
    }
    Local<Value> result = script->Run();
    if (result.IsEmpty()) {
      trycatch.ReThrow();
      return;
    }
    args.GetReturnValue().Set(result);
  }


  ~ContextifyScript() {
    script_.Dispose();
  }
};

Persistent<FunctionTemplate> ContextifyContext::js_tmpl;
Persistent<FunctionTemplate> ContextifyContext::data_wrapper_tmpl;
Persistent<Function> ContextifyContext::data_wrapper_ctor;

Persistent<FunctionTemplate> ContextifyScript::script_tmpl;

void InitContextify(Local<Object> target) {
  HandleScope scope(node_isolate);
  ContextifyContext::Init(target);
  ContextifyScript::Init(target);
}

}  // namespace node

NODE_MODULE(node_contextify, node::InitContextify);
