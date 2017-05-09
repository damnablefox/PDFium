// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/edit/cpdf_objectstream.h"

#include "core/fpdfapi/edit/cpdf_creator.h"
#include "core/fpdfapi/edit/cpdf_encryptor.h"
#include "core/fpdfapi/edit/cpdf_flateencoder.h"
#include "core/fpdfapi/parser/fpdf_parser_utility.h"

namespace {

const int kObjectStreamMaxLength = 256 * 1024;

}  // namespace

CPDF_ObjectStream::CPDF_ObjectStream() : m_dwObjNum(0), m_index(0) {}

CPDF_ObjectStream::~CPDF_ObjectStream() {}

bool CPDF_ObjectStream::IsNotFull() const {
  return m_Buffer.GetLength() < kObjectStreamMaxLength;
}

void CPDF_ObjectStream::Start() {
  m_Items.clear();
  m_Buffer.Clear();
  m_dwObjNum = 0;
  m_index = 0;
}

void CPDF_ObjectStream::CompressIndirectObject(uint32_t dwObjNum,
                                               const CPDF_Object* pObj) {
  m_Items.push_back({dwObjNum, m_Buffer.GetLength()});
  m_Buffer << pObj;
}

void CPDF_ObjectStream::CompressIndirectObject(uint32_t dwObjNum,
                                               const uint8_t* pBuffer,
                                               uint32_t dwSize) {
  m_Items.push_back({dwObjNum, m_Buffer.GetLength()});
  m_Buffer.AppendBlock(pBuffer, dwSize);
}

FX_FILESIZE CPDF_ObjectStream::End(CPDF_Creator* pCreator) {
  ASSERT(pCreator);

  if (m_Items.empty())
    return 0;

  CFX_FileBufferArchive* pFile = pCreator->GetFile();
  FX_FILESIZE ObjOffset = pCreator->GetOffset();
  if (!m_dwObjNum)
    m_dwObjNum = pCreator->GetNextObjectNumber();

  CFX_ByteTextBuf tempBuffer;
  for (const auto& pair : m_Items)
    tempBuffer << pair.objnum << " " << pair.offset << " ";

  int32_t len = pFile->AppendDWord(m_dwObjNum);
  if (len < 0)
    return -1;
  pCreator->IncrementOffset(len);

  len = pFile->AppendString(" 0 obj\r\n<</Type /ObjStm /N ");
  if (len < 0)
    return -1;

  pCreator->IncrementOffset(len);
  uint32_t iCount = pdfium::CollectionSize<uint32_t>(m_Items);
  len = pFile->AppendDWord(iCount);
  if (len < 0)
    return -1;
  pCreator->IncrementOffset(len);

  if (pFile->AppendString("/First ") < 0)
    return -1;

  len = pFile->AppendDWord(static_cast<uint32_t>(tempBuffer.GetLength()));
  if (len < 0)
    return -1;
  if (pFile->AppendString("/Length ") < 0)
    return -1;
  pCreator->IncrementOffset(len + 15);

  tempBuffer << m_Buffer;

  CPDF_FlateEncoder encoder(tempBuffer.GetBuffer(), tempBuffer.GetLength(),
                            true, false);
  CPDF_Encryptor encryptor(pCreator->GetCryptoHandler(), m_dwObjNum,
                           encoder.GetData(), encoder.GetSize());

  len = pFile->AppendDWord(encryptor.GetSize());
  if (len < 0)
    return -1;

  pCreator->IncrementOffset(len);
  if (pFile->AppendString("/Filter /FlateDecode") < 0)
    return -1;

  pCreator->IncrementOffset(20);
  len = pFile->AppendString(">>stream\r\n");
  if (len < 0)
    return -1;
  if (pFile->AppendBlock(encryptor.GetData(), encryptor.GetSize()) < 0)
    return -1;

  pCreator->IncrementOffset(len + encryptor.GetSize());
  len = pFile->AppendString("\r\nendstream\r\nendobj\r\n");
  if (len < 0)
    return -1;

  pCreator->IncrementOffset(len);
  return ObjOffset;
}
