// Copyright 2014 the V8 project authors. All rights reserved.
// AUTO-GENERATED BY tools/generate-runtime-tests.py, DO NOT MODIFY
// Flags: --allow-natives-syntax --harmony --harmony-proxies
var _break_id = 32;
var _wrapped_id = 1;
var _inlined_jsframe_index = 32;
var _source = "foo";
var _disable_break = true;
var _context_extension = new Object();
try {
%DebugEvaluate(_break_id, _wrapped_id, _inlined_jsframe_index, _source, _disable_break, _context_extension);
} catch(e) {}
