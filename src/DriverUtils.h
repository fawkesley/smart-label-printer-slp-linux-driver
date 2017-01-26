/*
 * "$Id: DriverUtils.h,v 1.6 Selznick$"
 *
 *   This file contains the prototypes for the bitmap manipulation commands.
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

#if !TARGET_API_MAC_OSX

typedef char *Ptr;

struct Rect
{
	short top;
	short left;
	short bottom;
	short right;
};
typedef struct Rect Rect;
typedef Rect *RectPtr;

struct BitMap
{
	Ptr baseAddr;
	short rowBytes;
	Rect bounds;
};

typedef struct BitMap BitMap;
typedef BitMap *BitMapPtr;
typedef BitMapPtr *BitMapHandle;

typedef unsigned char UInt8;
typedef signed char SInt8;
typedef unsigned short UInt16;
typedef signed short SInt16;
typedef unsigned long UInt32;
typedef signed long SInt32;
#endif

const BitMap kEmptyBitMap = {};

bool BitMapCreate(BitMap &outBitMap, char *inBaseAddress, SInt32 inRowBytes, Rect &inBounds);
void BitMapDispose(BitMap &ioBitMap);
bool BitMapRotate90(BitMap &srcBitMap, BitMap &outDestBitMap);
char *BitMapGetLinePtr(BitMap &inBitMap, int inLine);

/*
 * End of "$Id: DriverUtils.h,v 1.6 Selznick$"
 */
