# SiglusExtract
SiglusExtract : A tool that can extract almost all resources used by SiglusEngine and repack some of them for translation.

## OpenSource License  

![Files](https://www.gnu.org/graphics/gplv3-127x51.png)

All source code files are licensed under [GNU General Public License v3 (GPLv3)](https://www.gnu.org/licenses/quick-guide-gplv3.en.html).  

## Note
SiglusExtract has stopped developing.  
If you have any needs, pls develop by yourself.  
  

## How to build
1. VS2013 (or higher)  
2. WDK (Windows7, Windows8, Windows 8.1)
3. CMake


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
- Archives: *.ovk, *.nwk
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
(2)	Has Encryption: file has additional encryption.  
(SiglusExtract can detect whether additional encryption exists, so manually changing this setting isn’t necessary.)

6.	Repack Scene.pck:  
Specify a directory containing all .ss script files for the currently loaded game. You may also specify output file name.  
Click on “Do Pack!” to start packing.  

7.	Repack Gameexe.dat  
Similarly, specify decrypted Gameexe.txt (must be encoded with UTF16-LE w/ BOM).  

8.	Universal Patch  
Inherit Icon: Inherit icon used by game. Patched game executable will use this icon.  
Click on “Make Universal Patch” to create patch.  

### Notice:  (Incompatible with old version)   
Now Universal Patch will do almost everything for you, including:  
1. Bypass SiglusEngine's DVD check  
2. Bypass Japanese font check(BUT : pls choose monospaced font)   
3. Bypass Font distance check  

Also, some settings can be set via config file(*.ini)  
BUT pls copy 'SiglusExtractCfg.exe' to your game folder then use this tool to set everything  
1. Customize the name of script and configuration file (eg : Scene-en.pck & Gameexe-en.dat)  
2. Set global font(some games have no interface to select font)  



## List of Specialized Tools

1.	Omv2Avi.exe  
There are two types of OMV video files, 24-bit and 32-bit. 32-bit OMV files incorporate a specially-encoded alpha channel (to record transparency), and thus cannot be correctly played by most video players and editors. Omv2Avi, when fed with a 32-bit OMV file, will convert it into an uncompressed 32-bit AVI video file (with transparency data preserved), which can be easily edited in most video editing softwares. (If you feed a 24-bit OMV file instead, Omv2Avi will convert it into a standard Ogv video file, just as SiglusExtract would.)  
Please note: uncompressed AVI video files take up colossal amounts of disk space. Before you convert, please ensure that 1) the file system on your hard drive supports single files larger than 4 gigabytes; and that 2) you have lots of free disk space.

2.	vaconv  
vanconv is a component of RLdev, a RealLive (predecessor to SiglusEngine) resource extracting and repacking tool developed by Haeleth. You can use vaconv if your translation project requires you to edit game images and pack them back into g00 format. For a detailed guide on how to use vaconv, run it in command prompt without parameters.

3.	G00packMax.exe (located inside g00Pack_official folder)  
G00packMax is an official tool developed by Visual Art’s itself, capable of packing BMP and PSD images into g00 files. In some situations, vaconv may fail to properly repack; when this happens, please use G00packMax instead.
For example, when one g00 file contains multiple images whose coordinates overlap, vaconv will only be able to pack the last image of the series into g00, because it overwrites the previous image each time a new image is added.  
![Main Window](https://github.com/xmoeproject/SiglusExtract/blob/master/images/g00.jpg)  
Instead, pack the series of images, in its correct order, into one PSD file, and insert an empty (transparent) layer named #CUT as the top layer (see image above for illustration). The last step is to repack this PSD file using G00packMax. For more info on how to use G00packMax, please read the text files under the g00Pack_official folder (in Japanese).


## Notes  

**SiglusExtract is currently under beta testing, and may have stability issues. More Specialized tools will be added in the future.**

## Special thanks
- User guide translated by cklodar