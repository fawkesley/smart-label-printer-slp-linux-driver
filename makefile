#
# "$Id: makefile,v 1.6 Selznick$"
#   Makefile for Seiko Instruments USA Inc. Smart Label Printer Driver
#
#   Copyright yyyy-YYYY by Easy Software Products, all rights
#   reserved.
#
#   These coded instructions, statements, and computer programs are
#   the property of Easy Software Products and are protected by
#   Federal copyright law.  Distribution and use rights are outlined
#   in the file "LICENSE.txt" which should have been included with
#   this file.  If this file is missing or damaged please contact
#   Easy Software Products at:
#
#       Attn: CUPS Licensing Information
#       Easy Software Products
#       44141 Airport View Drive, Suite 204
#       Hollywood, Maryland 20636 USA
#
#       Voice: (301) 373-9600
#       EMail: cups-info@cups.org
#         WWW: http://www.cups.org/

mfdir     := $(shell pwd)
program   := seikoslp.rastertolabel
ppddir    := $(shell cups-config --datadir)/model/seiko
filterdir := $(shell cups-config --serverbin)/filter
cflags    := $(shell cups-config --ldflags --cflags)
ldflags   := $(shell cups-config --image --libs)

build:
	make clean
	$(CXX) -o $(program) $(cflags) *.cxx $(ldflags)
	# set up the filter directory in the ppd correctly.
	perl -p -i -e 's(^.cupsFilter.*\Z) <*cupsFilter: "application/vnd.cups-raster 0 $(filterdir)/$(program)">g' siislp100.ppd
	perl -p -i -e 's(^.cupsFilter.*\Z) <*cupsFilter: "application/vnd.cups-raster 0 $(filterdir)/$(program)">g' siislp200.ppd
	perl -p -i -e 's(^.cupsFilter.*\Z) <*cupsFilter: "application/vnd.cups-raster 0 $(filterdir)/$(program)">g' siislp240.ppd
	perl -p -i -e 's(^.cupsFilter.*\Z) <*cupsFilter: "application/vnd.cups-raster 0 $(filterdir)/$(program)">g' siislp440.ppd
	perl -p -i -e 's(^.cupsFilter.*\Z) <*cupsFilter: "application/vnd.cups-raster 0 $(filterdir)/$(program)">g' siislp450.ppd
	perl -p -i -e 's(^.cupsFilter.*\Z) <*cupsFilter: "application/vnd.cups-raster 0 $(filterdir)/$(program)">g' siislp620.ppd
	perl -p -i -e 's(^.cupsFilter.*\Z) <*cupsFilter: "application/vnd.cups-raster 0 $(filterdir)/$(program)">g' siislp650.ppd
	# set up the icon directory in the ppd correctly.
	perl -p -i -e "s#\(^.APPrinterIconPath.*\$\)\n##g" siislp100.ppd
	perl -p -i -e "s#\(^.APPrinterIconPath.*\$\)\n##g" siislp200.ppd
	perl -p -i -e "s#\(^.APPrinterIconPath.*\$\)\n##g" siislp240.ppd
	perl -p -i -e "s#\(^.APPrinterIconPath.*\$\)\n##g" siislp440.ppd
	perl -p -i -e "s#\(^.APPrinterIconPath.*\$\)\n##g" siislp450.ppd
	perl -p -i -e "s#\(^.APPrinterIconPath.*\$\)\n##g" siislp620.ppd
	perl -p -i -e "s#\(^.APPrinterIconPath.*\$\)\n##g" siislp650.ppd

install:
	make build
	mv $(program) "$(filterdir)/"
	mkdir "$(ppddir)"
	gzip -c siislp100.ppd >> siislp100.ppd.gz
	gzip -c siislp200.ppd >> siislp200.ppd.gz
	gzip -c siislp240.ppd >> siislp240.ppd.gz
	gzip -c siislp440.ppd >> siislp440.ppd.gz
	gzip -c siislp450.ppd >> siislp450.ppd.gz
	gzip -c siislp620.ppd >> siislp620.ppd.gz
	gzip -c siislp650.ppd >> siislp650.ppd.gz
	mv *.ppd.gz "$(ppddir)"

uninstall:
	rm -rfv "$(ppddir)"
	rm -rfv "$(filterdir)/$(program)"

clean:
	rm -f $(program) *.o *~
	rm -f *.gz
	rm -rf "$(ppddir)"
	rm -rf "$(mfdir)/pretty"

archive:
	make clean
	rm -f "$(mfdir)/../SeikoSLPLinuxDriver.zip"
	ditto -c -k --keepParent "$(mfdir)" "$(mfdir)/../SeikoSLPLinuxDriver.zip"
	
test300:
	#test at 300 dpi
	lp -d SLP450 -o scaling=100 SLP2RL-300-outline.png

test203:
	#test at 203 dpi
	lp -d SLP240-430 -o scaling=100 SLP2RL-203-outline.png

pretty:
	# pretty up the source code files using bcpp.  (C++ compatible pretty printer.)
	make clean
	mkdir "$(mfdir)/pretty"
	mv *.h "$(mfdir)/pretty/"
	mv *.cxx "$(mfdir)/pretty/"
	bcpp "$(mfdir)/pretty/SIISLPProcessBitmap.cxx" "$(mfdir)/SIISLPProcessBitmap.cxx"
	bcpp "$(mfdir)/pretty/SeikoSLPCommands.h" "$(mfdir)/SeikoSLPCommands.h"
	bcpp "$(mfdir)/pretty/SIISLPProcessBitmap.h" "$(mfdir)/SIISLPProcessBitmap.h"
	bcpp "$(mfdir)/pretty/RasterToSIISLP.h" "$(mfdir)/RasterToSIISLP.h"
	bcpp "$(mfdir)/pretty/DriverUtils.h" "$(mfdir)/DriverUtils.h"
	bcpp "$(mfdir)/pretty/RasterToSIISLP.cxx" "$(mfdir)/RasterToSIISLP.cxx"
	bcpp "$(mfdir)/pretty/DriverUtils.cxx" "$(mfdir)/DriverUtils.cxx"
	bcpp "$(mfdir)/pretty/SeikoInstrumentsVendorID.h" "$(mfdir)/SeikoInstrumentsVendorID.h"
	rm -rf "$(mfdir)/pretty"
	
#
# End of "$Id: makefile,v 1.6 Selznick$"
#
