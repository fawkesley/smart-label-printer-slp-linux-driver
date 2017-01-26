/*
 * "$Id: SeikoSLPCommands.h,v 1.6 Selznick$"
 *
 *   This file contains enumerated command IDs.
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

typedef enum
{
	SLP_CMD_NOP = 0x00,
	SLP_CMD_STATUS = 0x01,
	SLP_CMD_VERSION = 0x02,
	SLP_CMD_BAUDRATE = 0x03,
	SLP_CMD_PRINT = 0x04,
	SLP_CMD_PRINTRLE = 0x05,
	SLP_CMD_MARGIN = 0x06,
	SLP_CMD_REPEAT = 0x07,
	SLP_CMD_TAB = 0x09,
	SLP_CMD_LINEFEED = 0x0A,
	SLP_CMD_VERTTAB = 0x0B,
	SLP_CMD_FORMFEED = 0x0C,
	SLP_CMD_SETSPEED = 0x0D,
	SLP_CMD_DENSITY = 0x0E,
	SLP_CMD_RESET = 0x0F,
	SLP_CMD_MODEL = 0x12,
	SLP_CMD_INDENT = 0x16,
	SLP_CMD_FINEMODE = 0x17,
	SLP_CMD_SETSERIALNUM = 0x1B,
	SLP_CMD_CHECK = 0xA5,
} SLP_COMMANDS;

/*
 * End of "$Id: SeikoSLPCommands.h,v 1.6 Selznick$"
 */
