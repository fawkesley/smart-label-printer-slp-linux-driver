/*
 * "$Id: RasterToSIISLP.h,v 1.6 Selznick$"
 *
 *   This file contains prototypes for main routines.
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

enum
{
	stringPageProgress,
	stringNoPages,
	stringPrinterReady
};

/*  Prototypes... */
void    PreparePrinter();
void    StartPage(cups_page_header_t &header);
void    EndPage(void);
void    CancelJob(int sig);

/*  Utilities */
void    PageToBitmap(int pageNum, cups_raster_t *ras, cups_page_header_t &header, BitMap &oBitmap);
void    OutputStringToCUPS(SInt32 id, SInt32 i1, SInt32 i2);
void    DumpArgs(int argc, char *argv[]);
int     ComputeIndent(cups_page_header_t &header);
void    SetSignal(void);
void    ResetSignal(void);
void    SendPrinterReset(void);
void    ParseSelectedUserOptions(char *argv5);

int     main(int argc, char *argv[]);

#define DEBUG_FUNC  { fprintf(stderr, "S3: %s\n", __PRETTY_FUNCTION__); fflush(stderr); }
#define DEBUG_LINE  { fprintf(stderr, "S3: %s line is = %d\n", __PRETTY_FUNCTION__, __LINE__); fflush(stderr); }
#define DEBUG_S(S_) { fprintf(stderr, "S3: %s line %d, %s\n", __PRETTY_FUNCTION__, __LINE__, S_); fflush(stderr); }

/*
 * End of "$Id: RasterToSIISLP.h,v 1.6 Selznick$"
 */
