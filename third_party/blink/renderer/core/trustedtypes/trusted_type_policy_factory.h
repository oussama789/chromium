// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TRUSTEDTYPES_TRUSTED_TYPE_POLICY_FACTORY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TRUSTEDTYPES_TRUSTED_TYPE_POLICY_FACTORY_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/heap_allocator.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ExceptionState;
class ScriptState;
class ScriptValue;
class TrustedTypePolicy;
class TrustedTypePolicyOptions;

class CORE_EXPORT TrustedTypePolicyFactory final : public ScriptWrappable,
                                                   public ContextClient {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(TrustedTypePolicyFactory);

 public:
  static TrustedTypePolicyFactory* Create(ExecutionContext* context) {
    return MakeGarbageCollected<TrustedTypePolicyFactory>(context);
  }

  explicit TrustedTypePolicyFactory(ExecutionContext*);

  // TrustedTypePolicyFactory.idl
  TrustedTypePolicy* createPolicy(const String&,
                                  const TrustedTypePolicyOptions*,
                                  bool exposed,
                                  ExceptionState&);

  TrustedTypePolicy* getExposedPolicy(const String&);

  Vector<String> getPolicyNames() const;

  bool isHTML(ScriptState*, const ScriptValue&);
  bool isScript(ScriptState*, const ScriptValue&);
  bool isScriptURL(ScriptState*, const ScriptValue&);
  bool isURL(ScriptState*, const ScriptValue&);

  void Trace(blink::Visitor*) override;

 private:
  const WrapperTypeInfo* GetWrapperTypeInfoFromScriptValue(ScriptState*,
                                                           const ScriptValue&);

  HeapHashMap<String, Member<TrustedTypePolicy>> policy_map_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TRUSTEDTYPES_TRUSTED_TYPE_POLICY_FACTORY_H_
