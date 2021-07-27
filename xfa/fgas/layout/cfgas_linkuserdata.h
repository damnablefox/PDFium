// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FGAS_LAYOUT_CFGAS_LINKUSERDATA_H_
#define XFA_FGAS_LAYOUT_CFGAS_LINKUSERDATA_H_

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/retain_ptr.h"

class CFGAS_LinkUserData final : public Retainable {
 public:
  CONSTRUCT_VIA_MAKE_RETAIN;

  const wchar_t* GetLinkURL() const { return m_wsURLContent.c_str(); }

 private:
  explicit CFGAS_LinkUserData(const WideString& wsText);
  ~CFGAS_LinkUserData() override;

  WideString m_wsURLContent;
};

#endif  // XFA_FGAS_LAYOUT_CFGAS_LINKUSERDATA_H_
