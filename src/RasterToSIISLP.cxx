/*
 * "$Id: RasterToSIISLP.cxx,v 1.6 Selznick$"
 *
 *   This file contains the functions to gather the raster data from
 *   CUPS, reformat it, and pipe it to stdout for transfer to the
 *   printers.
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

/*
	Debugging notes:
	Messages beginning with "S3:" written to stderr, can be viewed in
	/var/log/cups/error_log file if the CUPS debug level is set to
	"debug".
*/

#include <cups/cups.h>
#include <cups/raster.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>

#include "SeikoSLPCommands.h"
#include "SeikoInstrumentsVendorID.h"
#include "SIISLPProcessBitmap.h"
#include "DriverUtils.h"
#include "RasterToSIISLP.h"

#include <signal.h>

#define contains(_target, _query) (NULL != strstr(_target, _query))

/*  Globals... */
int     gPrinterModel = 0;
double  gDotsPerLine = 0;
int     gFineMode = true;
int     gPrintDensity = 0;

/*  Install signal handler for errors or user cancel */
void SetSignal(void)
{
	#if defined(HAVE_SIGACTION) && !defined(HAVE_SIGSET)
	struct sigaction action;	 /*  Actions for POSIX signals */
	#endif

	#ifdef HAVE_SIGSET			 /*  Use System V signals over POSIX to avoid bugs */
	sigset(SIGTERM, CancelJob);
	#elif defined(HAVE_SIGACTION)
	memset(&action, 0, sizeof(action));

	sigemptyset(&action.sa_mask);
	action.sa_handler = CancelJob;
	sigaction(SIGTERM, &action, NULL);
	#else
	signal(SIGTERM, CancelJob);
	#endif
}


/*  Remove signal handlers */
void ResetSignal(void)
{
	#if defined(HAVE_SIGACTION) && !defined(HAVE_SIGSET)
	struct sigaction action;	 /*  Actions for POSIX signals */
	#endif

	#ifdef HAVE_SIGSET			 /*  Use System V signals over POSIX to avoid bugs */
	sigset(SIGTERM, SIG_IGN);
	#elif defined(HAVE_SIGACTION)
	memset(&action, 0, sizeof(action));

	sigemptyset(&action.sa_mask);
	action.sa_handler = SIG_IGN;
	sigaction(SIGTERM, &action, NULL);
	#else
	signal(SIGTERM, SIG_IGN);
	#endif
}


/*  Compute the indent for the printer we're printing to. */
int ComputeIndent(cups_page_header_t &header)
{
	if (0 == gPrinterModel)
	{
		exit(1);
	}

	double marginInDots = gDotsPerLine - header.cupsHeight;
	int marginInMM = 0;

	switch (gPrinterModel)
	{
		case kSeikoInstrumentsSLP100ProductID:
		case kSeikoInstrumentsSLP200ProductID:
		case kSeikoInstrumentsSLP240ProductID:
        case kSeikoInstrumentsSLP620ProductID:
			/*  8 dots == 1 mm, 8 dots == 1 byte */
			marginInMM = (int) ((marginInDots + 7) / 16);
			break;
		case kSeikoInstrumentsSLP440ProductID:
		case kSeikoInstrumentsSLP450ProductID:
        case kSeikoInstrumentsSLP650ProductID:
			/*  11.811 dots == 1 mm, 8 dots == 1 byte */
			marginInMM = (int) ((marginInDots + 7) / 23.622);
			break;
	}

	fprintf(stderr, "S3: gDotsPerLine %f, cupsHeight %d, margInInMM %d (Negative will be 0)\n",
		gDotsPerLine, header.cupsHeight, marginInMM);
	fflush(stderr);

	if (marginInMM < 0)
	{
		marginInMM = 0;
	}

	/*
	 * The SLP 240, if the left margin is zero, the printer will set
	 *  the margin to about .75 inches.  So, we check for 0 and bump it to 1
	 *  to work around this.
	 *  This does not appear to be the case anymore.  Our major code
	 *  refactoring for 1.1 has fixed all this.
	 */
	if (0 == marginInMM)
	{
		/*  marginInMM = 1; */
	}

	return marginInMM;
}


void SendPrinterCommand(char inCommand)
{
	char base[2] = {0};
	base[0] = inCommand;

	fwrite(base, 1, 1, stdout);
	fflush(stdout);
}


void SendPrinterCommand(char inCommand, char inArg1)
{
	char base[3] = {0};
	base[0] = inCommand;
	base[1] = inArg1;

	fwrite(base, 2, 1, stdout);
	fflush(stdout);
}


void SendPrinterCommand(char inCommand, char *inBuffer, int inBufferLength)
{
	#define MAX_SII_BUFF_SIZE 256

	if (inBufferLength + 2 > MAX_SII_BUFF_SIZE)
	{
		return;
	}

	char base[MAX_SII_BUFF_SIZE] = {0};
	base[0] = inCommand;
	base[1] = inBufferLength;
	::memcpy(&base[2], inBuffer, inBufferLength);

	fwrite(base, inBufferLength + 2, 1, stdout);
	fflush(stdout);
}


void SendPrinterReset(void)
{
	SendPrinterCommand(SLP_CMD_RESET);
	sleep(3);
}


/*
 * We don't know the model number of the printer we're communicating with so we first
 * pull the PPD from the environment variable and then open that PPD file.  The model
 * number is specified in the PPD file and is placed into ppd_file_t by CUPS.
 * The unique model numbers defined in the PPD files are actually the USB ProductIDs
 * of the printers.
 */
void PreparePrinter()
{
	DEBUG_FUNC;

	{
		const char *ppdPath = ::getenv("PPD");
		ppd_file_t *ppdFile = ::ppdOpenFile(ppdPath);
		if (NULL == ppdFile)
		{
			DEBUG_S("Can't find PPD file.");
			exit(1);
		}

		gPrinterModel = ppdFile->model_number;

		char debug[128];
		sprintf(debug, "model_number is %d", ppdFile->model_number);
		DEBUG_S(debug);

		ppdClose(ppdFile);
	}

	switch (gPrinterModel)
	{
		case kSeikoInstrumentsSLP100ProductID:
			gDotsPerLine = 192;
			break;
		case kSeikoInstrumentsSLP200ProductID:
		case kSeikoInstrumentsSLP240ProductID:
        case kSeikoInstrumentsSLP620ProductID:
			gDotsPerLine = 384;
			break;
		case kSeikoInstrumentsSLP440ProductID:
		case kSeikoInstrumentsSLP450ProductID:
        case kSeikoInstrumentsSLP650ProductID:
			gDotsPerLine = 576;
			break;
		default:
			DEBUG_S("Unknown printer!");
			exit(1);
			break;
	}

	if (kSeikoInstrumentsSLP100ProductID == gPrinterModel)
	{
		SendPrinterReset();
	}
}


/*  'StartPage()' - Start a page of graphics. */
void StartPage(cups_page_header_t &header)
{
	DEBUG_FUNC;

	SetSignal();

	int marginInMM = ComputeIndent(header);
	SendPrinterCommand(SLP_CMD_MARGIN, marginInMM);
	SendPrinterCommand(SLP_CMD_DENSITY, gPrintDensity);

	switch (gPrinterModel)
	{
		case kSeikoInstrumentsSLP100ProductID:
		case kSeikoInstrumentsSLP200ProductID:
		case kSeikoInstrumentsSLP240ProductID:
			SendPrinterCommand(SLP_CMD_FINEMODE, gFineMode);
			break;
		case kSeikoInstrumentsSLP440ProductID:
		case kSeikoInstrumentsSLP450ProductID:
        case kSeikoInstrumentsSLP620ProductID:
        case kSeikoInstrumentsSLP650ProductID:
			if (gFineMode)
			{
				SendPrinterCommand(SLP_CMD_SETSPEED, 0x02);
			}
			else
			{
				SendPrinterCommand(SLP_CMD_SETSPEED, 0x00);
			}
			break;
	}

}


void PageToBitmap(int pageNum, cups_raster_t *ras, cups_page_header_t &header, BitMap &oBitmap)
{
	std::vector<unsigned char> bitmapvector;

	/*  Loop for each line on the page... */
	for (int y = 0; y < header.cupsHeight; y++)
	{
		if (0 == (y % 15))
		{
			/*  Report progress to UI */
			OutputStringToCUPS(stringPageProgress, pageNum, 100 * y / header.cupsHeight);
		}

		/*  Read a line of graphics... */
		{
			std::vector<unsigned char> buffer(header.cupsBytesPerLine);

			/*  Read a line of graphics... */
			if (cupsRasterReadPixels(ras, &buffer[0], header.cupsBytesPerLine) < 1)
			{
				break;
			}

			bitmapvector.insert(bitmapvector.end(), buffer.begin(), buffer.end());
		}
	}

	/*  We've accumulated an entire label.  Convert the handle to a bitmap, and print the label. */
	Rect r = {0};
	r.top = 0;
	r.left = 0;
	r.bottom = header.cupsHeight;
	r.right = header.cupsWidth;

	BitMapCreate(oBitmap, (char *) &bitmapvector[0], header.cupsBytesPerLine, r);
}


/*  'EndPage()' - Finish a page of graphics. */
void EndPage(void)
{
	DEBUG_FUNC;

	/*  Unregister the signal handler. */
	ResetSignal();
}


/*  'CancelJob()' - Cancel the current job. */
void CancelJob(int /* sig */)
{
	DEBUG_FUNC;

	/*  End the current page and exit... */
	EndPage();

	exit(0);
}


/*  Dump the command-line arguments to standard error. */
void DumpArgs(int argc, char *argv[])
{
	long *endian = (long *) "\1\2\3\4";
	fprintf(stderr, "S3: endian flag 0x%0x\n", *endian);
	fflush(stderr);

	#if 0
	fprintf(stderr, "S3: argc = %d\n", argc);

	/*  On 10.2 CUPS, the following lines will prevent the driver from running. */
	char **v = argv;
	while (argc)
	{
		fprintf(stderr, "S3: argv[%d]==%s\n", v - argv, *v);
		fflush(stderr);
		v += 1;
		argc -= 1;
	}
	#endif

}


#if !TARGET_API_MAC_OSX

/*  Output a localized error to standard error to be picked up by CUPS.  This is a "public" error. */
void OutputStringToCUPS(SInt32 id, SInt32 i1, SInt32 i2)
{
	char *master = NULL;
	char result[128] = "";

	/*  At the end of each case "result" will contain a message for CUPS. */
	switch (id)
	{
		case stringPageProgress:
			master = "INFO: Printing page %d, %d%% complete...";
			sprintf(result, master, i1, i2);
			break;

		case stringNoPages:
			strcpy(result, "ERROR: No pages found!");
			break;

		case stringPrinterReady:
			master = "INFO: %s is ready to print.";

			switch (gPrinterModel)
			{
				case kSeikoInstrumentsSLP100ProductID:
					sprintf(result, master, "SLP100/410");
					break;

				case kSeikoInstrumentsSLP200ProductID:
					sprintf(result, master, "SLP200/420");
					break;

				case kSeikoInstrumentsSLP240ProductID:
					sprintf(result, master, "SLP240/430");
					break;

				case kSeikoInstrumentsSLP440ProductID:
					sprintf(result, master, "SLP440");
					break;

				case kSeikoInstrumentsSLP450ProductID:
					sprintf(result, master, "SLP450");
					break;
                    
                case kSeikoInstrumentsSLP620ProductID:
                    sprintf(result, master, "SLP620");
                    break;
                
                case kSeikoInstrumentsSLP650ProductID:
                    sprintf(result, master, "SLP650/SLP650SE");
                    break;
				
				default:
					sprintf(result, master, "Unknown");
					break;
			}
			break;
	}

	fprintf(stderr, "%s\n", result);
	fflush(stderr);
}


#else

/*  For output to cups on OSX, we have to localize all strings though the CF* functions.
 *  No such manipulations are necessary on Linux. */
void OutputStringToCUPS(SInt32 id, SInt32 i1, SInt32 i2)
{
	CFStringRef master = NULL;
	CFStringRef result = NULL;

	switch (id)
	{
		case stringPageProgress:
			master = CFCopyLocalizedString(
				CFSTR("INFO: Printing page %d, %d%% complete..."),
				CFSTR("Leave 'INFO: ' prefix.  This is the progress dialog.  The first %d will be replaced with a page number, the second %d%% will appear as 22%"));

			result = CFStringCreateWithFormat(NULL, NULL, master, i1, i2);
			break;

		case stringNoPages:
			result = CFCopyLocalizedString(
				CFSTR("ERROR: No pages found!"),
				CFSTR("Leave 'ERROR: ' prefix."));

			break;

		case stringPrinterReady:
			master = CFCopyLocalizedString(
				CFSTR("INFO: %@ is ready to print."),
				CFSTR("Leave 'INFO: ' %@ will be replaced by the printer name."));

			switch (gPrinterModel)
			{
				case kSeikoInstrumentsSLP100ProductID:
					result = CFStringCreateWithFormat(NULL, NULL, master, CFSTR("SLP100/410"));
					break;

				case kSeikoInstrumentsSLP200ProductID:
					result = CFStringCreateWithFormat(NULL, NULL, master, CFSTR("SLP200/420"));
					break;

				case kSeikoInstrumentsSLP240ProductID:
					result = CFStringCreateWithFormat(NULL, NULL, master, CFSTR("SLP240/430"));
					break;

				case kSeikoInstrumentsSLP440ProductID:
					result = CFStringCreateWithFormat(NULL, NULL, master, CFSTR("SLP440"));
					break;

				case kSeikoInstrumentsSLP450ProductID:
					result = CFStringCreateWithFormat(NULL, NULL, master, CFSTR("SLP450"));
					break;
				
				case kSeikoInstrumentsSLP620ProductID:
					result = CFStringCreateWithFormat(NULL, NULL, master, CFSTR("SLP620"));
					break;
                    
				case kSeikoInstrumentsSLP650ProductID:
					result = CFStringCreateWithFormat(NULL, NULL, master, CFSTR("SLP650/SLP650SE"));
					break;
                    
				default:
					result = CFStringCreateWithFormat(NULL, NULL, master, CFSTR("Unknown"));
					break;
			}
			break;
	}

	char buffer[256];
	Boolean ok = CFStringGetCString(result, buffer, 256, kCFStringEncodingUTF8);
	if (ok)
	{
		fprintf(stderr, "%s\n", buffer);
		fflush(stderr);
	}

	if (NULL != master)
	{
		CFRelease(master);
	}

	if (NULL != result)
	{
		CFRelease(result);
	}
}
#endif

/*
 * CUPS passes the user's print dialog selection to argv[5] into main.  We analyze it here.
 * All arguments, including argv[5] are dumped in DumpArgs.
 */
void ParseSelectedUserOptions(char *argv5)
{
	/*  Fine Print */
	gFineMode = not contains(argv5, "noFinePrint");
	fprintf(stderr, "S3: gFineMode = %d\n", gFineMode);

	/*  Density */
	if (contains(argv5, "Density=LowQuality"))
	{
        switch (gPrinterModel) {
            case kSeikoInstrumentsSLP100ProductID:
            case kSeikoInstrumentsSLP200ProductID:
            case kSeikoInstrumentsSLP240ProductID:
            case kSeikoInstrumentsSLP440ProductID:
            case kSeikoInstrumentsSLP450ProductID:
                gPrintDensity = 0xFA; /* 70% */
                break;
            case kSeikoInstrumentsSLP620ProductID:
            case kSeikoInstrumentsSLP650ProductID:
            default:
                gPrintDensity = 0xF9; /*65% */
                break;
        }
		fprintf(stderr, "S3: Low Quality\n");
	}
	else if (contains(argv5, "Density=MediumQuality"))
	{
		gPrintDensity = 0x00;	 /*  100% */
		fprintf(stderr, "S3: Medium Quality\n");
	}
	else
	{
		gPrintDensity = 0x06;	 /*  130% */
		fprintf(stderr, "S3: High Quality\n");
	}

	/*  Rotation... no. */
	/*  if (contains(argv5, "media=35mmSlide")) { } */

	fflush(stderr);
}


/*  'main()' - Main entry and processing of driver. */
int main(int argc, char *argv[])
{
	/*  Make sure status messages are not buffered... */
	setbuf(stderr, NULL);
	DEBUG_FUNC;

	DumpArgs(argc, argv);

	/*  Validate command-line... */
	if (argc < 6 || argc > 7)
	{
		fprintf(stderr, "ERROR: rastertosiislp job-id user title copies options [file]\n");
		fflush(stderr);
		return 1;
	}

	ParseSelectedUserOptions(argv[5]);

	/*  Open the page stream... */
	int  fd = 0;
	if (argc == 7)
	{
		if (-1 == (fd = open(argv[6], O_RDONLY)))
		{
			fprintf(stderr, "ERROR: Unable to open raster file - %s", argv[6]);
			fflush(stderr);
			return 1;
		}
	}

	/*  Raster stream for printing */
	cups_raster_t *ras = cupsRasterOpen(fd, CUPS_RASTER_READ);

	/*  Initialize the print device... */
	PreparePrinter();

	int pageNum = 0;
	cups_page_header_t  header = /*  Page header from file */
	{
		0
	};
	while (cupsRasterReadHeader(ras, &header))
	{

		/*  Write a status message with the page number and number of copies. */
		pageNum += 1;

		fprintf(stderr, "PAGE: %d %d\n", pageNum, header.NumCopies);
		fflush(stderr);

		StartPage(header);

		{
			BitMap bm = kEmptyBitMap;

			PageToBitmap(pageNum, ras, header, bm);

			/*  rotate to "landscape" */
			BitMapRotate90(bm, bm);

			SIISLPProcessBitmap processor(gPrinterModel);

			long height = bm.bounds.bottom - bm.bounds.top;
			long width  = bm.bounds.right  - bm.bounds.left;
			processor.ProcessBitmap(height, width, bm.rowBytes, (Byte *) bm.baseAddr);

			for (int copies = 0; copies < header.NumCopies; copies += 1)
			{
				fprintf(stderr, "S3: %d Copies remaining\n", copies);
				fflush(stderr);

				fwrite(processor.GetProcessedBytes(), processor.GetProcessedBytesSize(), 1, stdout);
				fflush(stdout);
			}

			BitMapDispose(bm);
		}

		EndPage();
	}

	/*  Close the raster stream... */
	cupsRasterClose(ras);
	if (0 != fd)
	{
		close(fd);
	}

	/*  If no pages were printed, send an error message... */
	if (0 == pageNum)
	{
		OutputStringToCUPS(stringNoPages, 0, 0);
	}
	else
	{
		OutputStringToCUPS(stringPrinterReady, 0, 0);
	}

	fprintf(stderr, "S3: Exit %s\n", __PRETTY_FUNCTION__); fflush(stderr);
	return (0 == pageNum);
}


/*
 * End of "$Id: RasterToSIISLP.cxx,v 1.6 Selznick$"
 */
