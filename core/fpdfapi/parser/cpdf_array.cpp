// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/parser/cpdf_array.h"

#include <set>
#include <utility>

#include "core/fpdfapi/parser/cpdf_boolean.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fxcrt/fx_stream.h"
#include "third_party/base/check.h"
#include "third_party/base/containers/contains.h"
#include "third_party/base/notreached.h"

CPDF_Array::CPDF_Array() = default;

CPDF_Array::CPDF_Array(const WeakPtr<ByteStringPool>& pPool) : m_pPool(pPool) {}

CPDF_Array::~CPDF_Array() {
  // Break cycles for cyclic references.
  m_ObjNum = kInvalidObjNum;
  for (auto& it : m_Objects) {
    if (it->GetObjNum() == kInvalidObjNum)
      it.Leak();
  }
}

CPDF_Object::Type CPDF_Array::GetType() const {
  return kArray;
}

bool CPDF_Array::IsArray() const {
  return true;
}

CPDF_Array* CPDF_Array::AsMutableArray() {
  return this;
}

const CPDF_Array* CPDF_Array::AsArray() const {
  return this;
}

RetainPtr<CPDF_Object> CPDF_Array::Clone() const {
  return CloneObjectNonCyclic(false);
}

RetainPtr<CPDF_Object> CPDF_Array::CloneNonCyclic(
    bool bDirect,
    std::set<const CPDF_Object*>* pVisited) const {
  pVisited->insert(this);
  auto pCopy = pdfium::MakeRetain<CPDF_Array>();
  for (const auto& pValue : m_Objects) {
    if (!pdfium::Contains(*pVisited, pValue.Get())) {
      std::set<const CPDF_Object*> visited(*pVisited);
      if (auto obj = pValue->CloneNonCyclic(bDirect, &visited))
        pCopy->m_Objects.push_back(std::move(obj));
    }
  }
  return pCopy;
}

CFX_FloatRect CPDF_Array::GetRect() const {
  CFX_FloatRect rect;
  if (m_Objects.size() != 4)
    return rect;

  rect.left = GetNumberAt(0);
  rect.bottom = GetNumberAt(1);
  rect.right = GetNumberAt(2);
  rect.top = GetNumberAt(3);
  return rect;
}

CFX_Matrix CPDF_Array::GetMatrix() const {
  if (m_Objects.size() != 6)
    return CFX_Matrix();

  return CFX_Matrix(GetNumberAt(0), GetNumberAt(1), GetNumberAt(2),
                    GetNumberAt(3), GetNumberAt(4), GetNumberAt(5));
}

absl::optional<size_t> CPDF_Array::Find(const CPDF_Object* pThat) const {
  for (size_t i = 0; i < size(); ++i) {
    if (GetDirectObjectAt(i) == pThat)
      return i;
  }
  return absl::nullopt;
}

bool CPDF_Array::Contains(const CPDF_Object* pThat) const {
  return Find(pThat).has_value();
}

RetainPtr<CPDF_Object> CPDF_Array::GetMutableObjectAt(size_t index) {
  return pdfium::WrapRetain(const_cast<CPDF_Object*>(GetObjectAt(index)));
}

const CPDF_Object* CPDF_Array::GetObjectAt(size_t index) const {
  if (index >= m_Objects.size())
    return nullptr;
  return m_Objects[index].Get();
}

RetainPtr<CPDF_Object> CPDF_Array::GetMutableDirectObjectAt(size_t index) {
  return pdfium::WrapRetain(const_cast<CPDF_Object*>(GetDirectObjectAt(index)));
}

const CPDF_Object* CPDF_Array::GetDirectObjectAt(size_t index) const {
  const CPDF_Object* pObj = GetObjectAt(index);
  return pObj ? pObj->GetDirect() : nullptr;
}

ByteString CPDF_Array::GetStringAt(size_t index) const {
  if (index >= m_Objects.size())
    return ByteString();
  return m_Objects[index]->GetString();
}

WideString CPDF_Array::GetUnicodeTextAt(size_t index) const {
  if (index >= m_Objects.size())
    return WideString();
  return m_Objects[index]->GetUnicodeText();
}

bool CPDF_Array::GetBooleanAt(size_t index, bool bDefault) const {
  if (index >= m_Objects.size())
    return bDefault;
  const CPDF_Object* p = m_Objects[index].Get();
  return ToBoolean(p) ? p->GetInteger() != 0 : bDefault;
}

int CPDF_Array::GetIntegerAt(size_t index) const {
  if (index >= m_Objects.size())
    return 0;
  return m_Objects[index]->GetInteger();
}

float CPDF_Array::GetNumberAt(size_t index) const {
  if (index >= m_Objects.size())
    return 0;
  return m_Objects[index]->GetNumber();
}

RetainPtr<CPDF_Dictionary> CPDF_Array::GetMutableDictAt(size_t index) {
  return pdfium::WrapRetain(const_cast<CPDF_Dictionary*>(GetDictAt(index)));
}

const CPDF_Dictionary* CPDF_Array::GetDictAt(size_t index) const {
  const CPDF_Object* p = GetDirectObjectAt(index);
  if (!p)
    return nullptr;
  if (const CPDF_Dictionary* pDict = p->AsDictionary())
    return pDict;
  if (const CPDF_Stream* pStream = p->AsStream())
    return pStream->GetDict();
  return nullptr;
}

RetainPtr<CPDF_Stream> CPDF_Array::GetMutableStreamAt(size_t index) {
  return pdfium::WrapRetain(const_cast<CPDF_Stream*>(GetStreamAt(index)));
}

const CPDF_Stream* CPDF_Array::GetStreamAt(size_t index) const {
  return ToStream(GetDirectObjectAt(index));
}

RetainPtr<CPDF_Array> CPDF_Array::GetMutableArrayAt(size_t index) {
  return pdfium::WrapRetain(const_cast<CPDF_Array*>(GetArrayAt(index)));
}

const CPDF_Array* CPDF_Array::GetArrayAt(size_t index) const {
  return ToArray(GetDirectObjectAt(index));
}

void CPDF_Array::Clear() {
  CHECK(!IsLocked());
  m_Objects.clear();
}

void CPDF_Array::RemoveAt(size_t index) {
  CHECK(!IsLocked());
  if (index < m_Objects.size())
    m_Objects.erase(m_Objects.begin() + index);
}

void CPDF_Array::ConvertToIndirectObjectAt(size_t index,
                                           CPDF_IndirectObjectHolder* pHolder) {
  CHECK(!IsLocked());
  if (index >= m_Objects.size())
    return;

  if (!m_Objects[index] || m_Objects[index]->IsReference())
    return;

  CPDF_Object* pNew = pHolder->AddIndirectObject(std::move(m_Objects[index]));
  m_Objects[index] = pNew->MakeReference(pHolder);
}

CPDF_Object* CPDF_Array::SetAt(size_t index, RetainPtr<CPDF_Object> pObj) {
  CHECK(!IsLocked());
  CHECK(pObj);
  CHECK(pObj->IsInline());
  if (index >= m_Objects.size())
    return nullptr;

  CPDF_Object* pRet = pObj.Get();
  m_Objects[index] = std::move(pObj);
  return pRet;
}

CPDF_Object* CPDF_Array::InsertAt(size_t index, RetainPtr<CPDF_Object> pObj) {
  CHECK(!IsLocked());
  CHECK(pObj);
  CHECK(pObj->IsInline());
  if (index > m_Objects.size())
    return nullptr;

  CPDF_Object* pRet = pObj.Get();
  m_Objects.insert(m_Objects.begin() + index, std::move(pObj));
  return pRet;
}

CPDF_Object* CPDF_Array::Append(RetainPtr<CPDF_Object> pObj) {
  CHECK(!IsLocked());
  CHECK(pObj);
  CHECK(pObj->IsInline());
  CPDF_Object* pRet = pObj.Get();
  m_Objects.push_back(std::move(pObj));
  return pRet;
}

bool CPDF_Array::WriteTo(IFX_ArchiveStream* archive,
                         const CPDF_Encryptor* encryptor) const {
  if (!archive->WriteString("["))
    return false;

  for (size_t i = 0; i < size(); ++i) {
    if (!GetObjectAt(i)->WriteTo(archive, encryptor))
      return false;
  }
  return archive->WriteString("]");
}

CPDF_ArrayLocker::CPDF_ArrayLocker(const CPDF_Array* pArray)
    : m_pArray(pArray) {
  m_pArray->m_LockCount++;
}

CPDF_ArrayLocker::~CPDF_ArrayLocker() {
  m_pArray->m_LockCount--;
}
