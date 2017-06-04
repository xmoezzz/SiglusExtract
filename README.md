# SiglusExtract
SiglusExtract : A tool that can extract almost all resources used by SiglusEngine and repack some of them for translation.

## How to use
- SiglusExtract works in more or less the same way as KrkrExtract.

![Files](https://github.com/xmoeproject/SiglusExtract/blob/master/images/1st.jpg)

- First, copy the two files above, SiglusExtract.exe and SiglusExtract.dll, into game directory.
(Please note: if you have applied to the game an older version of AlphaROMdiE, which modifies game files, the modification could cause a conflict. Please restore the original executable before using SiglusExtract. If you have used a newer version of AlphaROMdiE, such as the latest Build 20140214, you should be fine.)
- Once both files are in game directory, double-click on SiglusExtract.exe or drag game executable onto SiglusExtract.exe to launch.

![Main Window](https://github.com/xmoeproject/SiglusExtract/blob/master/images/siglusext1.jpg)

- If no error occurs and you are greeted with the window shown above, then congratulations, you have successfully launched SiglusExtract.
Drag various game resource files onto this window to start unpacking.
- You may drag single / multiple files or folders at once.

## Supported resource types
- Images: *.g00, *.g01
- Videos: *.omv
- Audio: *.nwa, *.owp
- Archives: *.owk, *.nwk
- Script: Scene.pck (file must be in this exact name)
- Config: Gameexe.dat (file must be in this exact name)

## Features


1.	G00 Image:  
(1)	bmp: extract as BMP image file;  
(2)	png: extract as PNG image file;  
(3)	jpg: extract as JPEG image file.  

2.	NWA Audio:  
(1)	wav: extract as WAV audio file;  
(2)	flac: extract as FLAC audio file;  
(3)	vorbis: extract as Vorbis-encoded Ogg container audio file.  

3.	OWP:  
(1)	Decryption: decrypt into Ogg file;  
(2)	Raw: copy without decrypting.  

4.	OMV Movie:  
(1)	normal ogv: extract as regular Ogv video file, which can be played by most video players;  
(2)	png sequence: extract as a PNG sequence. If the original OMV video contains an alpha channel, this option will help you correctly extract it.  

5.	Extract Text:  
(1)	No Encryption: file has no additional encryption;  
(2)	Has Encryption: file has addition encryption.  
(SiglusExtract can detect whether additional encryption exists, so manually changing this setting isn’t necessary.)

6.	Repack Scene.pck:  
Specify a directory containing all .ss script files for the currently loaded game. You may also specify output file name.  
Click on “Do Pack!” to start packing.  

7.	Repack Gameexe.dat  
Similarly, specify decrypted Gameexe.txt (must be encoded with UTF16-LE w/ BOM).  

8.	Universal Patch  
Inherit Icon: Inherit icon used by game. Patched game executable will use this icon.  
Click on “Make Universal Patch” to create patch.  


**SiglusExtract is currently under beta testing, and may have stability issues. Specialized tools will be added in the future.**

## Special thanks
- cklodar