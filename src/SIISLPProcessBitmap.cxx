/*
 * "$Id: SIISLPProcessBitmap.cxx,v 1.6 Selznick$"
 *
 *   This file contains a class to optimize the size of
 *   the raster image for very efficient communications
 *   with the printers.
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

#include "stdlib.h"
#include "string.h"

#pragma warning( disable : 4800)
#endif

#include "SeikoInstrumentsVendorID.h"
#include "SeikoSLPCommands.h"
#include "SIISLPProcessBitmap.h"

inline long FirstError(long error1, long error2)
{
	long    status2 = error2;

	if (SIISLPNoError != error1)
		return error1;

	return status2;
}


#pragma mark "Public API"

SIISLPProcessBitmap::SIISLPProcessBitmap(UInt16 usbPID)
{
	mUSBPID = usbPID;
	mLastPrintCommandSize = 0;
	mProcessedBytes = 0;
	mProcessedBytesSize = 0;
	mUsePrintCompression = true;
}


SIISLPProcessBitmap::~SIISLPProcessBitmap()
{
	if (mProcessedBytes)
		free(mProcessedBytes);
}


long SIISLPProcessBitmap::ProcessBitmap(long height, long width, long rowBytes, Byte *bytes)
{
	OSStatus    result = SIISLPNoError;
	UInt16      linesToAdvance = 0, mask = 0;
	BytePtr     curLineData;
	Byte        lineBuffer[kMaxBytesPerLine];
	SInt32      curLine, bytesPerLine;
	SInt32      maxDotsPerLine;

	result = GetMaxDotsPerLine(maxDotsPerLine);

	if ((SIISLPNoError == result) && (width > maxDotsPerLine))
	{
		width = maxDotsPerLine;
	}
	if ((SIISLPNoError == result) && ((width/8) > kMaxBytesPerLine))
	{
		/*  Need to increase kMaxBytesPerLine! */
		result = kSIISLPImplementationError;
	}
	if (SIISLPNoError == result)
	{
		if (SIISLPNoError == result)
		{
			bytesPerLine = width / 8;
			if (width % 8)
				bytesPerLine++;

			if (mProcessedBytes)
				free(mProcessedBytes);
			/*  2 for print command and length, 100 for prepare & topofform */
			mProcessedBytesAllocSize = ((bytesPerLine + 2) * height) + 100;
			mProcessedBytes = (Byte *)malloc(mProcessedBytesAllocSize);
			if (!mProcessedBytes)
				result = kSIISLPMemFullError;
			else
				result = PrintPrepare(width);

			if (SIISLPNoError == result)
			{
				mask = GetBitMask(width);

				for (curLine = 1, curLineData = (BytePtr)bytes; (SIISLPNoError == result) && (curLine <= height); curLine++, curLineData += rowBytes)
				{
					bool    bLineEmpty = true;

					memcpy(lineBuffer, curLineData, bytesPerLine);
					lineBuffer[bytesPerLine - 1] = lineBuffer[bytesPerLine - 1] & mask;

					/*  Look to see if this line is empty */
					for (SInt32 pos = 0; pos < bytesPerLine; pos++)
					{
						if (lineBuffer[pos] != 0)
						{
							bLineEmpty = false;
							break;
						}
					}
					if (bLineEmpty)
					{
						linesToAdvance++;
					}
					else
					{
						/*  Advance any previous empty lines */
						if (linesToAdvance)
						{
							result = AdvanceLines(linesToAdvance);
							linesToAdvance = 0;
						}
						/*  Print this line */
						if (SIISLPNoError == result)
						{
							result = ProcessLine(lineBuffer, bytesPerLine);
						}
					}
				}
			}
		}

		Byte        topOfForm = SLP_CMD_FORMFEED;

		result = FirstError(result, AppendBytes(&topOfForm, 1));
	}

	return result;
}


Byte * SIISLPProcessBitmap::GetProcessedBytes()
{
	return mProcessedBytes;
}


size_t SIISLPProcessBitmap::GetProcessedBytesSize()
{
	return mProcessedBytesSize;
}


void SIISLPProcessBitmap::SetCompression(bool bUsePrintCompression)
{
	mUsePrintCompression = bUsePrintCompression;
}


#pragma mark "Private API"

long SIISLPProcessBitmap::GetMaxDotsPerLine(long & maxDotsPerLine)
{
	OSStatus    result = SIISLPNoError;

	switch (mUSBPID)
	{
        case kSeikoInstrumentsSLP650ProductID:
		case kSeikoInstrumentsSLP440ProductID:
		case kSeikoInstrumentsSLP450ProductID:
			maxDotsPerLine = 576;
			break;
        case kSeikoInstrumentsSLP620ProductID:
		case kSeikoInstrumentsSLP200ProductID:
		case kSeikoInstrumentsSLP240ProductID:
			maxDotsPerLine = 384;
			break;
		case kSeikoInstrumentsSLP100ProductID:
			maxDotsPerLine = 192;
			break;
		default:
			result = kSIISLPUnknownUSBPIDError;
			break;
	}

	return result;
}


Byte SIISLPProcessBitmap::GetBitMask(size_t bitCount)
{
	Byte        mask = 0;

	switch (bitCount % 8)
	{
		case 0:
			mask = 0xFF;
			break;
		case 1:
			mask = 0x80;
			break;
		case 2:
			mask = 0xC0;
			break;
		case 3:
			mask = 0xE0;
			break;
		case 4:
			mask = 0xF0;
			break;
		case 5:
			mask = 0xF8;
			break;
		case 6:
			mask = 0xFC;
			break;
		case 7:
			mask = 0xFE;
			break;
	}
	return mask;
}


long SIISLPProcessBitmap::PrintPrepare(long width)
{
	OSStatus    result;
	SInt32      maxDotsPerLine;

	result = GetMaxDotsPerLine(maxDotsPerLine);
	if (SIISLPNoError == result)
	{
		SInt32      marginInDots = (maxDotsPerLine - width) / 2;
		SInt32      marginInMM = 0;
		Byte        command[2];

		if (marginInDots < 0)
			marginInDots = 0;

		switch (mUSBPID)
		{
            case kSeikoInstrumentsSLP650ProductID:
            case kSeikoInstrumentsSLP620ProductID:
			case kSeikoInstrumentsSLP440ProductID:
			case kSeikoInstrumentsSLP450ProductID:
				if (marginInDots <= 255)
				{
					command[0] = SLP_CMD_INDENT;
					command[1] = (Byte) marginInDots;
				}
				else
				{
					/*  11.81 pixels = 1mm on the 300 dpi SLP printers */
					marginInMM = ((marginInDots * 100) + 590) / 1181;
					command[0] = SLP_CMD_MARGIN;
					command[1] = (Byte) marginInMM;
				}
				break;
			case kSeikoInstrumentsSLP200ProductID:
			case kSeikoInstrumentsSLP240ProductID:
			case kSeikoInstrumentsSLP100ProductID:
				/*
				 * The older printers contain a bug when the CMD_INDENT command is used in combination with the CMD_TAB command
				 * so on the older printers, we only use CMD_MARGIN.
				 * 8 pixels = 1mm on the older SLP printers
				 */
				marginInMM = (marginInDots + 3) / 8;
				command[0] = SLP_CMD_MARGIN;
				command[1] = (Byte) marginInMM;
				break;
			default:
				result = kSIISLPUnknownUSBPIDError;
				break;
		}

		if (SIISLPNoError == result)
			AppendBytes(command, sizeof(command));
	}
	return result;
}


long SIISLPProcessBitmap::AdvanceLines(UInt16 linesToAdvance)
{
	Byte        buffer[2];
	OSStatus    result = SIISLPNoError;

	while ((linesToAdvance > 0) && (SIISLPNoError == result))
	{
		if (linesToAdvance == 1)
		{
			buffer[0] = SLP_CMD_LINEFEED;
			result = AppendBytes(buffer, 1);
			linesToAdvance--;
		}
		else if (linesToAdvance > 255)
		{
			buffer[0] = SLP_CMD_VERTTAB;
			buffer[1] = 255;
			result = AppendBytes(buffer, 2);
			linesToAdvance -= 255;
		}
		else
		{
			buffer[0] = SLP_CMD_VERTTAB;
			buffer[1] = (Byte) linesToAdvance;

			result = AppendBytes(buffer, 2);
			linesToAdvance = 0;
		}
	}

	return result;
}


void SIISLPProcessBitmap::CompressRun(Byte bufferCompressed[], int runCount, bool runIsBlack, Byte *bytes, int &byteIndex, int &bitIndex, size_t byteCount, unsigned int &bufferIndex, int &lengthIndex, bool &firstRun)
{
	int printCount;

	while (runCount > 0)
	{
		printCount = runCount;
		if (firstRun)
		{
			/*  Use CMD_TAB if it will be less bytes.  CMD_TAB is 2 bytes, so we only use it if the RLE run would be 3 bytes. */
			if ((!runIsBlack) && (runCount > 126))
			{
				if (printCount > 255)
					printCount = 255;
				bufferCompressed[bufferIndex++] = SLP_CMD_TAB;
				bufferCompressed[bufferIndex++] = (Byte) printCount;
				runCount -= printCount;
			}
			bufferCompressed[bufferIndex++] = SLP_CMD_PRINTRLE;
			lengthIndex = bufferIndex;
			bufferIndex++;
			firstRun = false;
		}
		else
		{
			/*  If a run is shorter than 8 bits, we're better off not compressing it, and taking the 7 bits at a time. */
			/*  But we can only do this if there are 8 bits of data left to print. */

			if ((printCount < 8) && (NULL != bytes) && ((((byteCount - 1 - byteIndex) * 8) + 8 - bitIndex) + printCount >= 7))
			{
				int bitCount;

				/*  First do the bits for this run */
				if (runIsBlack)
					bufferCompressed[bufferIndex] = 0xFF << (7 - printCount);
				else
					bufferCompressed[bufferIndex] = 128;

				/*  Now grab any needed extra bits (we need 7) */
				for (bitCount = printCount + 1; bitCount <= 7; bitCount++)
				{
					/*  If the next bit is black move it over.  If it's white we don't need to do anything. */
					if (bytes[byteIndex] & (1 << (7 - bitIndex)))
					{
						bufferCompressed[bufferIndex] = bufferCompressed[bufferIndex] |  (1 << (7 - bitCount));
					}
					bitIndex++;
					if (bitIndex == 8)
					{
						byteIndex++;
						bitIndex = 0;
					}
				}
			}
			else
			{
				if (printCount > 63)
					printCount = 63;
				if (runIsBlack)
					bufferCompressed[bufferIndex] = 64 + printCount;
				else
					bufferCompressed[bufferIndex] = (Byte) printCount;
			}
			runCount -= printCount;
			bufferIndex++;
		}
	}
}


/*
// This block of code doesn't do uncompressed runs.
void SIISLPProcessBitmap::CompressRun(Byte bufferCompressed[], int runCount, bool runIsBlack, Byte *, int &byteIndex, int &bitIndex, size_t, unsigned int &bufferIndex, int &lengthIndex, bool &firstRun)
{
	int	printCount;

	while (runCount > 0)
	{
		printCount = runCount;
		if (firstRun)
		{
			// Use CMD_TAB if it will be less bytes.  CMD_TAB is 2 bytes, so we only use it if the RLE run would be 3 bytes.
			if ((!runIsBlack) && (runCount > 126))
			{
				if (printCount > 255)
					printCount = 255;
				bufferCompressed[bufferIndex++] = SLP_CMD_TAB;
				bufferCompressed[bufferIndex++] = printCount;
				runCount -= printCount;
			}
			bufferCompressed[bufferIndex++] = SLP_CMD_PRINTRLE;
			lengthIndex = bufferIndex;
			bufferIndex++;
			firstRun = false;
		}
		else
		{
			if (printCount > 63)
				printCount = 63;
			if (runIsBlack)
				bufferCompressed[bufferIndex] = 64 + printCount;
			else
				bufferCompressed[bufferIndex] = printCount;
			runCount -= printCount;
			bufferIndex++;
		}
	}
}
*/

long SIISLPProcessBitmap::ProcessLine(Byte * bytes, size_t byteCount)
{
	long            result = SIISLPNoError;
	Byte            bufferCompressed[kMaxBytesPerLine];
	Byte            bufferRaw[kMaxBytesPerLine];
	Byte            repeat = SLP_CMD_REPEAT;
	int             compressedCount = 0;
	int             rawCount;
	unsigned int    bufferIndex = 0;

	/*
	 * Generate the compressed version
	 * We use a simplified form of compression.  They actually allow mixing of compressed and uncompressed data
	 * but that would get complicated to figure out and I'm just not gonna do it...
	 */
	if ((SIISLPNoError == result) && (true == mUsePrintCompression))
	{
		int             byteIndex;
		int             bitIndex, runCount;
		int             lengthIndex = 1;
		bool            runIsBlack;
		bool            bitIsBlack;
		bool            firstRun = true;

		runIsBlack = (bytes[0] & (1 << (7))) ? true : false;
		runCount = 1;
		bufferIndex = 0;
		for (byteIndex = 0, bitIndex = 1; (!(((byteIndex+1) == (int)byteCount) && (bitIndex == 8))) && ((byteIndex) < (int)byteCount); bitIndex++)
		{
			if (bitIndex == 8)
			{
				byteIndex++;
				bitIndex = 0;
			}
			bitIsBlack = (bytes[byteIndex] & (1 << (7 - bitIndex))) ? true : false;
			if (runIsBlack == bitIsBlack)
			{
				runCount++;
			}
			else
			{
				CompressRun(bufferCompressed, runCount, runIsBlack, bytes, byteIndex, bitIndex, byteCount, bufferIndex, lengthIndex, firstRun);

				/*  Figure out if we just finished off the data. */
				if (byteIndex < (int)byteCount)
				{
					runIsBlack = (bool) (bytes[byteIndex] & (1 << (7 - bitIndex)));
					runCount = 1;
				}
				else
					runCount = 0;
			}
		}

		/*  We don't need to add trailing white space, so we only print if the last run is black. */
		if ((runIsBlack) && (runCount))
		{
			CompressRun(bufferCompressed, runCount, runIsBlack, NULL, byteIndex, bitIndex, byteCount, bufferIndex, lengthIndex, firstRun);
		}

		/*  Look for trailing white space and strip it off */
		while (bufferIndex > 3)
		{
			if ((!(bufferCompressed[bufferIndex-1] & 0x40)) && (!(bufferCompressed[bufferIndex-1] & 0x80)))
				bufferIndex--;
			else
				break;
		}

		bufferCompressed[lengthIndex] = bufferIndex - lengthIndex - 1;

		compressedCount = bufferIndex;
	}

	if (SIISLPNoError == result)
	{
		/*  Now build the raw or uncompress print command */
		while (bytes[byteCount-1] == 0)
			byteCount--;

		bufferRaw[0] = SLP_CMD_PRINT;
		bufferRaw[1] = (Byte) byteCount;
		memcpy(&bufferRaw[2], bytes, byteCount);

		rawCount = byteCount +2;
	}

	/*  Only print the compressed version if compression is on, and it really works out to be smaller than the uncompressed version. */
	if ((SIISLPNoError == result) && (true == mUsePrintCompression) && (compressedCount < rawCount))
	{
		if ((compressedCount == mLastPrintCommandSize) && (0 == memcmp(bufferCompressed, mLastPrintCommand, mLastPrintCommandSize)))
			AppendBytes(&repeat, 1);
		else
			AppendBytes(bufferCompressed, compressedCount);
	}
	else if (SIISLPNoError == result)
	{
		if ((rawCount == mLastPrintCommandSize) && (0 == memcmp(bufferRaw, mLastPrintCommand, mLastPrintCommandSize)))
			AppendBytes(&repeat, 1);
		else
			AppendBytes(bufferRaw, rawCount);
	}

	return result;
}


long SIISLPProcessBitmap::AppendBytes(Byte * bytes, size_t bufferSize)
{
	long        result = SIISLPNoError;

	if ((!mProcessedBytes) || (mProcessedBytesSize + bufferSize > mProcessedBytesAllocSize))
		result = kSIISLPImplementationError;
	else
	{
		memcpy(&mProcessedBytes[mProcessedBytesSize], bytes, bufferSize);
		mProcessedBytesSize += bufferSize;
	}
	return result;
}


/*
 * End of "$Id: SIISLPProcessBitmap.cxx,v 1.6 Selznick$"
 */
