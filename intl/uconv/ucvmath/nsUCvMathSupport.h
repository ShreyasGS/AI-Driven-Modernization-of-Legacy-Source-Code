/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Roger B. Sidje <rbs@maths.uq.edu.au>
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsUCvMathSupport_h___
#define nsUCvMathSupport_h___

#include "nsIUnicodeEncoder.h"
#include "nsIUnicodeDecoder.h"
#include "nsIUnicodeEncodeHelper.h"
#include "nsIUnicodeDecodeHelper.h"
#include "nsICharRepresentable.h"

#define ONE_BYTE_TABLE_SIZE 256

//----------------------------------------------------------------------
// Class nsBasicDecoderSupport [declaration]

/**
 * Support class for the Unicode decoders. 
 *
 * The class source files for this class are in /ucvlatin/nsUCvMathSupport. 
 * However, because these objects requires non-xpcom subclassing, local copies
 * will be made into the other directories using them. Just don't forget to 
 * keep in sync with the master copy!
 * 
 * This class implements:
 * - nsISupports
 * - nsIUnicodeDecoder
 *
 * @created         19/Apr/1999
 * @author  Catalin Rotaru [CATA]
 */
class nsBasicDecoderSupport : public nsIUnicodeDecoder
{
  NS_DECL_ISUPPORTS

public:

  /**
   * Class constructor.
   */
  nsBasicDecoderSupport();

  /**
   * Class destructor.
   */
  virtual ~nsBasicDecoderSupport();

  //--------------------------------------------------------------------
  // Interface nsIUnicodeDecoder [declaration]
};

//----------------------------------------------------------------------
// Class nsBufferDecoderSupport [declaration]

/**
 * Support class for the Unicode decoders. 
 *
 * This class implements:
 * - the buffer management
 *
 * @created         15/Mar/1999
 * @author  Catalin Rotaru [CATA]
 */
class nsBufferDecoderSupport : public nsBasicDecoderSupport
{
protected:

  /**
   * Internal buffer for partial conversions.
   */
  char *    mBuffer;
  PRInt32   mBufferCapacity;
  PRInt32   mBufferLength;

  /**
   * Convert method but *without* the buffer management stuff.
   */
  NS_IMETHOD ConvertNoBuff(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength) = 0;

  void FillBuffer(const char ** aSrc, PRInt32 aSrcLength);
  void DoubleBuffer();

public:

  /**
   * Class constructor.
   */
  nsBufferDecoderSupport();

  /**
   * Class destructor.
   */
  virtual ~nsBufferDecoderSupport();

  //--------------------------------------------------------------------
  // Interface nsIUnicodeDecoder [declaration]

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength);
  NS_IMETHOD Reset();
};

//----------------------------------------------------------------------
// Class nsTableDecoderSupport [declaration]

/**
 * Support class for a single-table-driven Unicode decoder.
 * 
 * @created         15/Mar/1999
 * @author  Catalin Rotaru [CATA]
 */
class nsTableDecoderSupport : public nsBufferDecoderSupport
{
public:

  /**
   * Class constructor.
   */
  nsTableDecoderSupport(uShiftTable * aShiftTable, 
      uMappingTable * aMappingTable);

  /**
   * Class destructor.
   */
  virtual ~nsTableDecoderSupport();

protected:

  nsIUnicodeDecodeHelper    * mHelper;      // decoder helper object
  uShiftTable               * mShiftTable;
  uMappingTable             * mMappingTable;

  //--------------------------------------------------------------------
  // Subclassing of nsBufferDecoderSupport class [declaration]

  NS_IMETHOD ConvertNoBuff(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength);
};

//----------------------------------------------------------------------
// Class nsMultiTableDecoderSupport [declaration]

/**
 * Support class for a multi-table-driven Unicode decoder.
 * 
 * @created         24/Mar/1999
 * @author  Catalin Rotaru [CATA]
 */
class nsMultiTableDecoderSupport : public nsBufferDecoderSupport
{
public:

  /**
   * Class constructor.
   */
  nsMultiTableDecoderSupport(PRInt32 aTableCount, uRange * aRangeArray, 
      uShiftTable ** aShiftTable, uMappingTable ** aMappingTable);

  /**
   * Class destructor.
   */
  virtual ~nsMultiTableDecoderSupport();

protected:

  nsIUnicodeDecodeHelper    * mHelper;      // decoder helper object
  PRInt32                   mTableCount;
  uRange                    * mRangeArray;
  uShiftTable               ** mShiftTable;
  uMappingTable             ** mMappingTable;

  //--------------------------------------------------------------------
  // Subclassing of nsBufferDecoderSupport class [declaration]

  NS_IMETHOD ConvertNoBuff(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength);
};

//----------------------------------------------------------------------
// Class nsBufferDecoderSupport [declaration]

/**
 * Support class for a single-byte Unicode decoder.
 *
 * @created         19/Apr/1999
 * @author  Catalin Rotaru [CATA]
 */
class nsOneByteDecoderSupport : public nsBasicDecoderSupport
{
public:

  /**
   * Class constructor.
   */
  nsOneByteDecoderSupport(uShiftTable * aShiftTable, 
      uMappingTable * aMappingTable);

  /**
   * Class destructor.
   */
  virtual ~nsOneByteDecoderSupport();

protected:

  nsIUnicodeDecodeHelper    * mHelper;      // decoder helper object
  uShiftTable               * mShiftTable;
  uMappingTable             * mMappingTable;
  PRUnichar                 mFastTable[ONE_BYTE_TABLE_SIZE];

  //--------------------------------------------------------------------
  // Subclassing of nsBasicDecoderSupport class [declaration]

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength);
  NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength, 
      PRInt32 * aDestLength);
  NS_IMETHOD Reset();
};

//----------------------------------------------------------------------
// Class nsBasicEncoder [declaration]

class nsBasicEncoder : public nsIUnicodeEncoder, public nsICharRepresentable
{
  NS_DECL_ISUPPORTS

public:
  /**
   * Class constructor.
   */
  nsBasicEncoder();

  /**
   * Class destructor.
   */
  virtual ~nsBasicEncoder();

};
//----------------------------------------------------------------------
// Class nsEncoderSupport [declaration]

/**
 * Support class for the Unicode encoders. 
 *
 * This class implements:
 * - nsISupports
 * - the buffer management
 * - error handling procedure(s)
 *
 * @created         17/Feb/1999
 * @author  Catalin Rotaru [CATA]
 */
class nsEncoderSupport :  public nsBasicEncoder
{

protected:

  /**
   * Internal buffer for partial conversions.
   */
  char *    mBuffer;
  PRInt32   mBufferCapacity;
  char *    mBufferStart;
  char *    mBufferEnd;

  /**
   * Error handling stuff
   */
  PRInt32   mErrBehavior;
  nsIUnicharEncoder * mErrEncoder;
  PRUnichar mErrChar;

  /**
   * Convert method but *without* the buffer management stuff and *with* 
   * error handling stuff.
   */
  NS_IMETHOD ConvertNoBuff(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength);

  /**
   * Convert method but *without* the buffer management stuff and *without*
   * error handling stuff.
   */
  NS_IMETHOD ConvertNoBuffNoErr(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength) = 0;

  /**
   * Finish method but *without* the buffer management stuff.
   */
  NS_IMETHOD FinishNoBuff(char * aDest, PRInt32 * aDestLength);

  /**
   * Copy as much as possible from the internal buffer to the destination.
   */
  nsresult FlushBuffer(char ** aDest, const char * aDestEnd);

public:

  /**
   * Class constructor.
   */
  nsEncoderSupport();

  /**
   * Class destructor.
   */
  virtual ~nsEncoderSupport();

  //--------------------------------------------------------------------
  // Interface nsIUnicodeEncoder [declaration]

  NS_IMETHOD Convert(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength);
  NS_IMETHOD Finish(char * aDest, PRInt32 * aDestLength);
  NS_IMETHOD Reset();
  NS_IMETHOD SetOutputErrorBehavior(PRInt32 aBehavior, 
      nsIUnicharEncoder * aEncoder, PRUnichar aChar);

  //--------------------------------------------------------------------
  // Interface nsICharRepresentable [declaration]
  NS_IMETHOD FillInfo(PRUint32 *aInfo) = 0;
};

//----------------------------------------------------------------------
// Class nsTableEncoderSupport [declaration]

/**
 * Support class for a single-table-driven Unicode encoder.
 * 
 * @created         17/Feb/1999
 * @author  Catalin Rotaru [CATA]
 */
class nsTableEncoderSupport : public nsEncoderSupport
{
public:

  /**
   * Class constructor.
   */
  nsTableEncoderSupport(uShiftTable * aShiftTable, 
      uMappingTable  * aMappingTable);

  /**
   * Class destructor.
   */
  virtual ~nsTableEncoderSupport();
  NS_IMETHOD FillInfo( PRUint32 *aInfo);

protected:

  nsIUnicodeEncodeHelper    * mHelper;      // encoder helper object
  uShiftTable               * mShiftTable;
  uMappingTable             * mMappingTable;

  //--------------------------------------------------------------------
  // Subclassing of nsEncoderSupport class [declaration]

  NS_IMETHOD ConvertNoBuffNoErr(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength);
};

//----------------------------------------------------------------------
// Class nsMultiTableEncoderSupport [declaration]

/**
 * Support class for a multi-table-driven Unicode encoder.
 * 
 * @created         11/Mar/1999
 * @author  Catalin Rotaru [CATA]
 */
class nsMultiTableEncoderSupport : public nsEncoderSupport
{
public:

  /**
   * Class constructor.
   */
  nsMultiTableEncoderSupport(PRInt32 aTableCount, uShiftTable ** aShiftTable,
      uMappingTable  ** aMappingTable);

  /**
   * Class destructor.
   */
  virtual ~nsMultiTableEncoderSupport();
  NS_IMETHOD FillInfo( PRUint32 *aInfo);

protected:

  nsIUnicodeEncodeHelper    * mHelper;      // encoder helper object
  PRInt32                   mTableCount;
  uShiftTable               ** mShiftTable;
  uMappingTable             ** mMappingTable;

  //--------------------------------------------------------------------
  // Subclassing of nsEncoderSupport class [declaration]

  NS_IMETHOD ConvertNoBuffNoErr(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength);
};

#endif /* nsUCvMathSupport_h___ */
