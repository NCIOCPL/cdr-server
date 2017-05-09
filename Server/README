# Instructions for building the CDR server using Visual Studio 2013

## What you'll need:

 * Visual Studio 2013
 * bison 3.0.x+ (available from cygwin)
 * Subversion client
 * A connection to the internet
 * An account on the CBIIT subversion server

## Build steps (substitute local drive letter for X: below)

 1. Install expat
  - go to http://sourceforge.net/projects/expat
  - download the latest expat_win32bin package (an .exe file)
  - run the executable package, installing to X:\usr\expat

 2. Build Sablotron
  - Go to X:\usr
  - set CDRTRUNK=https://ncisvn.nci.nih.gov/svn/oce_cdr/trunk
  - svn export -q %CDRTRUNK%/ExternalPackages/Sablot
  - cd Sablot
  - nmake -f makefile.msvc > errs
  - examine errs file for problems

 3. Install xerces.
  - go to http://xerces.apache.org/xerces-c/download.cgi
  - download xerces-c-x.x.x.zip (where x.x.x is the
    latest release version of the package; e.g. 3.1.4)
  - unpack the zipfile
  - cd xerces-c-x.x.x\projects\Win32\VC12\xerces-all\all
  - MSBuild.exe all.vcxproj /p:Configuration=Release /p:Platform=Win32
  - MSBuild.exe all.vcxproj /p:Configuration=Debug /p:Platform=Win32
  - cd ../../../../.. [back to xerces-c-x.x.x]
  - mkdir X:\usr\xerces
  - mkdir X:\usr\xerces\include
  - mkdir X:\usr\xerces\include\xercesc
  - xcopy /s src\xercesc X:\usr\xerces\include\xercesc
  - mkdir X:\usr\xerces\bin
  - mkdir X:\usr\xerces\lib
  - copy build\Win32\VC12\Release\*.exe X:\usr\xerces\bin
  - copy build\Win32\VC12\Release\*.dll X:\usr\xerces\bin
  - copy build\Win32\VC12\Debug\*.dll X:\usr\xerces\bin
  - copy build\Win32\VC12\Debug\xerces-c*.pdb X:\usr\xerces\bin
  - copy build\Win32\VC12\Release\*.lib X:\usr\xerces\lib
  - copy build\Win32\VC12\Debug\*.lib X:\usr\xerces\lib
  - (if xerces version is not 3.1.x, fix library name in CDR server Makefile)

 4. Ensure that there is a path to each of the dlls. Add to the path:
  - X:\usr\expat\Bin
  - X:\usr\Sablot\bin
  - X:\usr\xerces\bin

 5. Export and build the server source code
  - svn export https://ncisvn.nci.nih.gov/svn/oce_cdr/trunk/Server
  - cd Server
  - nmake DRV=X:

In addition to the DLLs installed by the steps above, the following
two DLLs must be copied to the search path, if they are not already
installed (available from
http://www.microsoft.com/en-us/download/details.aspx?id=5555):

    msvcp100.dll
    msvcr100.dll

If you are testing the server on your own workstation, you will need
to set up an ODBC connection with the name 'cdr' and connection
parameters which connect with the CDR databases.  It is important
to make sure that you set up a 32-bit ODBC connection object.  The
easiest way to do that if you're running a 64-bit build of Windows
is to invoke the ODBC connection manager directly:

       %windir%\SysWOW64\odbcad32.exe

Additional requirements for running the CDR Server:

 * writeable X:/cdr/log
 * readable X:/etc/cdrdbpw
 * readable X:/etc/cdrapphosts.rc
 * readable X:/etc/cdrtier.rc (containing, e.g., "QA" or "DEV")
 * readable X:/etc/cdrenv.rc (containing "CBIIT")
 * readable X:/cdr/etc/hostname (use "localhost" for running on workstation)
 * X:/cdr/bin/CdrServer.exe (doesn't have to be real, just has to exist)
 * statable X:/cdr/bin/CdrService.exe (doesn't have to do anything, just exist)
