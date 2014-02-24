This program is Copyright 2012-14 Cyril Marlin and Mathieu Legris, and is distributed
under the terms of the MIT Public License.

This source code is inspired by Huffyuv codec, http://www.math.berkeley.edu/~benrg/huffyuv.html

Normal installation:
Right-click on lagarith.inf and select install to install the codec. If you
do not see an install option, make sure you extracted the files, and are using
the extracted lagarith.inf file.

Installing the 32bit version on Windows 64
Extract all the files to a folder
Open up a command prompt (start->run: cmd)
Change directories to sysWOW64 ("cd C:\windows\syswow64")
Type the following (replace example_path with the actual path to the extracted files), then hit enter: rundll32.exe setupapi.dll,InstallHinfSection DefaultInstall 0 c:\example_path\lagarith.inf
This method can also be used for installing other 32bit codecs such as Huffyuv that install via a .inf file.

To install under Windows 7, you will need to run cmd.exe as administrator

I have tested this code only under Visual C++ 2008.

If you find this source code useful or you have problems, please let
me know!

Official repository: https://code.google.com/p/lighthaar/
