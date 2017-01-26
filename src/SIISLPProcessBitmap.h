/*
 * "$Id: SIISLPProcessBitmap.h,v 1.6 Selznick$"
 *
 *   This file contains a class to optimize a raster images.
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

/*  We should probably make some sort of types header, but there aren't that many so... */

typedef unsigned short UInt16;
typedef unsigned long UInt32;
typedef long SInt32;
typedef unsigned char Byte;
typedef unsigned char *BytePtr;
typedef SInt32 OSStatus;		 /*  Might not be a perfect mapping, but it works since I'm dealing with COM errors. */
typedef long SInt32;
#endif

/*  Error Codes */
#define SIISLPNoError               0

#if TARGET_API_MAC_OSX
#define kSIISLPUnknownUSBPIDError   kUSBUnknownDeviceErr
#define kSIISLPMemFullError         memFullErr
#define kSIISLPImplementationError  unimpErr
#else
#define kSIISLPUnknownUSBPIDError   1
#define kSIISLPMemFullError         2
#define kSIISLPImplementationError  3
#endif

#define kMaxBytesPerLine        128

class SIISLPProcessBitmap
{
	public:
		SIISLPProcessBitmap(UInt16          usbPID);
		virtual                 ~SIISLPProcessBitmap();

		long                    ProcessBitmap(      long            height,
			long            width,
			long            rowBytes,
			Byte *          bytes);

		Byte *                  GetProcessedBytes();
		size_t                  GetProcessedBytesSize();

		/*  Compression defaults to on.  This allows you to turn it off */
		void                    SetCompression(     bool            bUsePrintCompression);

	private:
		long                    GetMaxDotsPerLine(  long &          maxDotsPerLine);

		Byte                    GetBitMask(         size_t          bitCount);

		long                    PrintPrepare(       long            width);

		long                    AdvanceLines(       UInt16          linesToAdvance);

		void                    CompressRun(        Byte            bufferCompressed[],
			int             runCount,
			bool            runIsBlack,
			Byte *          bytes,
			int &           byteIndex,
			int &           bitIndex,
			size_t          byteCount,
			unsigned int &  bufferIndex,
			int &           lengthIndex,
			bool &          firstRun);

		long                    ProcessLine(        Byte *          bytes,
			size_t          byteCount);

		long                    AppendBytes(        Byte *          bytes,
			size_t          bufferSize);

	private:
		bool                    mUsePrintCompression;
		UInt16                  mUSBPID;

		Byte *                  mProcessedBytes;
		size_t                  mProcessedBytesSize;
		size_t                  mProcessedBytesAllocSize;

		Byte                    mLastPrintCommand[kMaxBytesPerLine];
		int                     mLastPrintCommandSize;
};

/*
 * End of "$Id: SIISLPProcessBitmap.h,v 1.6 Selznick$"
 */
