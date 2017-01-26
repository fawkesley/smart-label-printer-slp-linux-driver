/*
 * "$Id: DriverUtils.cxx,v 1.6 Selznick$"
 *
 *   This file contains uncompressed bitmap manipulation commands.
 *
 *   Copyright 1997-2007 by Easy Software Products.
 *
 *   These coded instructions, statements, and computer programs are the
 *   property of Easy Software Products and are protected by Federal
 *   copyright law.  Distribution and use rights are outlined in the file
 *   "LICENSE.txt" which should have been included with this file.  If this
 *   file is missing or damaged please contact Easy Software Products
 *   at:
 *
 *       Attn: CUPS Licensing Information
 *       Easy Software Products
 *       44141 Airport View Drive, Suite 204
 *       Hollywood, Maryland 20636-3142 USA
 *
 *       Voice: (301) 373-9600
 *       EMail: cups-info@cups.org
 *         WWW: http://www.cups.org
 */

#include "stdlib.h"
#include "string.h"
#include "DriverUtils.h"

/*  Get pointer to zero index inLine from bitMap. */
char *BitMapGetLinePtr(BitMap &inBitMap, int inLine)
{
	if (inLine >= inBitMap.bounds.bottom - inBitMap.bounds.top)
	{
		return NULL;
	}
	if (inLine < 0)
	{
		return NULL;
	}

	char *lineBase = inBitMap.baseAddr + (inLine * inBitMap.rowBytes);
	return lineBase;
}


bool BitMapCreate(BitMap &outBitMap, char *inBaseAddress, SInt32 inRowBytes, Rect &inBounds)
{
	outBitMap = kEmptyBitMap;

	SInt32 size = inRowBytes * (inBounds.bottom - inBounds.top);

	outBitMap.baseAddr = (Ptr) ::malloc(size);
	if (NULL == outBitMap.baseAddr)
	{
		return false;
	}

	outBitMap.bounds   = inBounds;
	outBitMap.rowBytes = inRowBytes;

	/*  Now copy the buffer */
	if (inBaseAddress)
	{
		::memcpy(outBitMap.baseAddr, inBaseAddress, size);
	}

	return true;
}


void BitMapDispose(BitMap &ioBitMap)
{
	if (NULL != ioBitMap.baseAddr)
	{
		::free(ioBitMap.baseAddr);
	}

	ioBitMap = kEmptyBitMap;
}


/*  Rotate the source bitmap into the destination bitmap. */
/*  src and dest may refer to the same structure. */
bool BitMapRotate90(BitMap &srcBitMap, BitMap &outDestBitMap)
{
	UInt16 srcCols = srcBitMap.bounds.right  - srcBitMap.bounds.left;
	UInt16 srcRows = srcBitMap.bounds.bottom - srcBitMap.bounds.top;

	UInt16 destCols = srcRows;
	UInt16 destRows = srcCols;
	UInt16 destRowBytes = (destCols + 7) >> 3;

	/*  Each bit in one source row will turn into a whole row in the destination. */
	UInt32 destBuffSize = destRows * destRowBytes;
	Ptr    destBase     = (Ptr) ::calloc(destBuffSize, 1);
	if (NULL == destBase)
	{
		return false;
	}

	BitMap destBitMap = kEmptyBitMap;

	destBitMap.baseAddr = destBase;
	destBitMap.bounds.left = 0;
	destBitMap.bounds.top = 0;
	destBitMap.bounds.right = destCols;
	destBitMap.bounds.bottom = destRows;
	destBitMap.rowBytes = destRowBytes;

	for (short srcRow = 0; srcRow < srcRows; srcRow++)
	{
		for (short srcCol = 0; srcCol < srcCols; srcCol++)
		{
			/*  Get src value */
			UInt8 value = 0;
			{
				SInt32 srcCharIndex = srcRow * srcBitMap.rowBytes;
				srcCharIndex += (srcCol >> 3);
				UInt8 mask = 0x80 >> (srcCol & 0x07);
				value = srcBitMap.baseAddr[srcCharIndex] & mask;
			}

			if (value)
			{
				/*  Backwards from end of row */
				SInt32 destCol = (srcRows - srcRow) - 1;
				SInt32 destRow = srcCol;

				SInt32 destCharIndex = destRow * destBitMap.rowBytes;
				destCharIndex += (destCol >> 3);
				UInt8 mask = 0x80 >> (destCol & 0x07);

				/*  How should we handle bounds error? */
				if ((destCharIndex >= 0) and (destCharIndex < destBuffSize))
				{
					destBitMap.baseAddr[destCharIndex] |= mask;
				}
			}
		}
	}

	if (&srcBitMap == &outDestBitMap)
	{
		BitMapDispose(outDestBitMap);
	}

	outDestBitMap = destBitMap;

	return true;
}


/*
 * End of "$Id: DriverUtils.cxx,v 1.6 Selznick$"
 */
