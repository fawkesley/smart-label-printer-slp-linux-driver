Please review LICENSE.txt.

------------------------------------------------------------------------
About:

Enclosed in this archive please find:

	- Two halves of a CUPS 1.1 printer driver:
		- PPD files for the following Smart Label Printers.
			- SLP100/410
			- SLP200/420
			- SLP240/430
			- SLP440
			- SLP450
			- SLP620
			- SLP650/650SE
		- Source code to build a binary CUPS filter.
	- A makefile to build and install the filter.
	- This README.txt file.
	- A LICENSE.txt file.

	Smart Label Printers are produced by Seiko Instruments, Inc.
	http://www.siibusinessproducts.com

	This driver described herein has only been validated with a USB interface.

	This driver has been tested with SuSE, RedHat/Fedora and Ubuntu Linux.
  
------------------------------------------------------------------------
Building the Driver

CUPS libraries:

	Building this driver requires that CUPS development libraries are installed.  These libraries can be found within your Linux implementation's software installer.
	
	Ubuntu: libcupsys2-dev and libcupsimage2-dev
	Others: cups-devel and cups-libs

Other libraries:

	(-ljpeg) libjpeg can be downloaded and built from <http://www.ijg.org>.
	(-lz) libz can be downloaded and built from <http://www.zlib.net>.
	
	.tar.gz files can be unpacked with tar -vxfz <filename>.
	.tar files can be unpacked with tar -vxf <filename>.
	
	Configure with ./configure.
	
	Copy the resulting libraries (usually .a suffix) to a directory accessible to your linker, usually /usr/lib.
	
From the command-line, utter the following:

	sudo make build
  
After a successful build, the driver may be installed with:

	sudo make install
  
The driver may be uninstalled with:

	sudo make uninstall
	
	This will not remove printer instances recognized by the system.

------------------------------------------------------------------------
Using the Driver

	Use the CUPS administration tool located at http://localhost:631 to install the printer instance on your system.
	
	The manufacturer name contained within the enclosed PPD files is "SII", not "Seiko".  PPDs for "Seiko" printers are widely distributed with the "Foomatic" open source effort and are NOT the PPDs provided by Seiko Instruments Inc.

	Note about Smart Label Printer 220.  The very popular 220 was discontinued many years ago.  Although it is not directly supported by this driver, the PPD for the SLP200/420 may be assigned to it manually.  (CUPS will allow any PPD to be used with any port or printer.  Although it may not work for any port/printer combination.)
  

------------------------------------------------------------------------
Testing the Driver

	Contained within the CUPS administration tool for an installed printer instance is an option to print a test page.  Alternatively:

	Alternatively:

	For SLP100/410, SLP200/420, SLP240/430 and SLP 620 models (203 dpi):
	
		make test203
	
	For SLP440, SLP450 and SLP650/650SE models (300 dpi):
	
		make test300

	The test203 and test300 commands are designed for the SLP-2RL Small Address Label only.

------------------------------------------------------------------------
Troubleshooting

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
Problem: The "SII" manufacturer doesn't appear in the manufacturer list when attempting to discover the printer.

Verify that SELinux is disabled.  CUPS may not find the PPDs in their nested directory if SELinux or other security software is enabled.  

Watching the output of the 'make install' command will reveal the destination path for the ppd.gz files on your system.

If CUPS and the driver are installed properly, CUPS should be able to find the PPDs and present them as an option when creating the printer instance.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
Problem: Test labels are scaled all wrong.

Sometimes, when creating a printer instance, a Linux flavor will assign a paper size other than the default specified by the PPD.  Default paper size may be changed with the CUPS administration utility at http://localhost:631.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
Problem: Labels other than the test labels are cropped or scaled wrong.

The entire height of a label is usually smaller than the sum of the default header and footer sizes hard coded into most programs.  Some programs, when trying to calculate printable area, will calculate a negative result and crash.

Many flavors of Linux have a printer administration utility separate from the CUPS administration utility (available from http://localhost:631).  Many times the non-CUPS test page will have the scaling and/or margin problems described above.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
Problem: It's still not working.
  
The CUPS documentation describes a debug level that can be set with the CUPS configuration to change the detail written to the CUPS error_log file.  After setting this file, the CUPS daemon must be restarted.  On many Linux systems this can be done by uttering '/etc/init.d/cups restart' on the command-line.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
