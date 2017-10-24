/********************************************
**
**	nwa フォーマットについて
**
**		全体としては以下の構造を持つ
**		NWA Header
**		data offset index
**		data block<0>
**		data block<1>
**		...
**		data block<N>
**
**	NWA Header: ファイル先頭から 44 bytes
**		magic number などはないのでnwa ファイルかは
**		データの整合性から判断する必要がある
**		データは全て little endian で、
**		short(signed 2byte)または int(signed 4byte) である。
**
**		+00 short   channel 数(1/2)
**		+02 short   sample 一つあたりの bit 数(16)
**		+04 int     周波数(一秒あたりのデータ数)
**		+08 int     圧縮レベル：-1~5.2で最小のデータ、0で最大の復元度(-1は無圧縮rawデータとみなされる)
**		+12 int     ?
**		+16 int     ブロック数
**		+20 int     展開後のデータの大きさ(バイト単位)
**		+24 int     圧縮時のデータの大きさ(nwa ファイルの大きさ。バイト単位)
**		+28 int     サンプル数：展開後のデータ数(16bit dataなら short 単位==サンプル単位のデータの大きさ)
**		+32 int     データ１ブロックを展開した時のサンプル単位のデータ数
**		+36 int     最終ブロックを展開した時のサンプル単位のデータ数
**		+40 int     ?
**
**	data offset index
**		全ブロック数 x 4 byte のデータ
**		それぞれ int のデータが全ブロック数続いている
**
**		データブロックの先頭を指すファイル先頭からの位置(オフセット)
**		が格納されている
**
**	data block
**		長さは可変。展開することで一定の大きさをもつデータに展開される。
**		データはDPCM形式。元 PCM データが a,b,c ならば (a),b-a, c-b と
**		いった差分データが、仮数3-5bit,指数3bitの形式で保存されている。
**		結果的に、16bit のデータが多くの場合 6-8bit で格納される。
**		仮数のビット数は圧縮レベル0で5bit、圧縮レベル2で3bitとなる。
**		以下、圧縮レベル2の場合について話を進める。
**		モノラルの場合：
**			+00 short  ブロック内の最初のデータ
**			+02- bit stream
**		ステレオの場合：
**			+00 short  左(?)チャンネルの最初のデータ
**			+02 short  右(?)チャンネルの最初のデータ
**			+04- bit stream
**
**		差分データの精度が高くないので各ブロックの先頭で
**		正確なデータにより補正される(？)
**
**	bit stream
**		little endian
**		+0 - +2 : 指数
**		+3 - +5 : 仮数
**		の形式。例えば a,b,c という8bitデータがあれば、
**		a&0x07 : データ１の指数
**		(a>>3)&0x07 : データ１の仮数(signed ;
**		((b<<2)|(a>>6))&0x07 : データ２の指数
**		(b>>1)&0x07 : データ２の仮数
**		となる。
**		ただし、指数の値により仮数のbit数が変化することがある。
**		指数 = 1 - 6 の場合：
**			a=指数、b=仮数、p=前のデータとして、今回のデータd は
**			bの2bit目が立っている場合：
**				d = p - (b&3)<<(4+a)
**			立ってない場合：
**				d = p + (b&3)<<(4+a)
**		指数 = 0 の場合：仮数は存在しない(データは3bitとなる)
**			d = p
**			「智代アフター」の音声ファイル (complevel == 5) ではランレングス圧縮用に使われている。
**		指数 = 7
**			次の bit が立っている場合：
**				d = 0 (現在未使用)
**				(データは4bitとなる)
**			次の bit が立ってない場合：
**				complevel = 0,1,2:
**				   仮数 b = 6bit
**				   b の 5bit 目が立っている場合：
**					d = p - (b&0x1f)<<(4+7)
**				   立ってない場合：
**					d = p + (b&0x1f)<<(4+7)
**				   (データは10bitとなる)
**				complevel = 3,4,5:
**				   仮数 b = 8bit
**				   b の 7bit 目が立っている場合：
**					d = p - (b&0x7f)<<9
**				   立ってない場合：
**					d = p + (b&0x1f)<<9
**				   (データは10bitとなる)
**
**		圧縮レベルが異なる場合、たとえば圧縮レベル==0で
**			指数==1~6でdの最上位bitが立っている場合
**				d = p - (b&0x0f)<<(2+a)
**			指数==7でdの最上位bitが立っている場合
**				d = p - (b&0x7f)<<(2+7)
**				(b : 8bitなのでデータは12bitとなる)
**		のように、精度だけが変化するようになっている。
**
**	ヘッダ読み込みについてはNWAData::ReadHeader()参照
**	bit stream からのデータ展開については NWADecode()参照
**************************************************************
*/


#include "my.h"
#include<string.h>
#include "endian.hpp"
#include "StreamWriter.h"



inline int getbits(const char*& data, int& shift, int bits) {
	if (shift > 8) { data++; shift -= 8; }
	int ret = read_little_endian_short(data) >> shift;
	shift += bits;
	return ret & ((1 << bits) - 1); /* mask */
}

/* 指定された形式のヘッダをつくる */
const char* make_wavheader(int size, int channels, int bps, int freq) {
	static char wavheader[0x2c] = {
		'R', 'I', 'F', 'F',
		0, 0, 0, 0, /* +0x04: riff size*/
		'W', 'A', 'V', 'E',
		'f', 'm', 't', ' ',
		16, 0, 0, 0, /* +0x10 : fmt size=0x10 */
		1, 0,    /* +0x14 : tag : pcm = 1 */
		2, 0,    /* +0x16 : channels */
		0, 0, 0, 0, /* +0x18 : samples per second */
		0, 0, 0, 0, /* +0x1c : average bytes per second */
		0, 0,     /* +0x20 : block alignment */
		0, 0,     /* +0x22 : bits per sample */
		'd', 'a', 't', 'a',
		0, 0, 0, 0 };/* +0x28 : data size */
	write_little_endian_int(wavheader + 0x04, size + 0x24);
	write_little_endian_int(wavheader + 0x28, size);
	write_little_endian_short(wavheader + 0x16, channels);
	write_little_endian_short(wavheader + 0x22, bps);
	write_little_endian_int(wavheader + 0x18, freq);
	int byps = (bps + 7) >> 3;
	write_little_endian_int(wavheader + 0x1c, freq*byps*channels);
	write_little_endian_short(wavheader + 0x20, byps*channels);
	return wavheader;
}

/* NWA の bitstream展開に必要となる情報 */
class NWAInfo {
	int channels;
	int bps;
	int complevel;
	bool use_runlength;
public:
	NWAInfo(int c, int b, int cl, bool rl) {
		channels = c;
		bps = b;
		complevel = cl;
		use_runlength = rl;
	}
	int Channels(void) const{ return channels; }
	int Bps(void) const { return bps; }
	int CompLevel(void) const { return complevel; }
	int UseRunLength(void) const { return use_runlength; }
};

template<class NWAI> void NWADecode(const NWAI& info, const char* data, char* outdata, int datasize, int outdatasize) {
	int d[2];
	int i;
	int shift = 0;
	const char* dataend = data + datasize;
	/* 最初のデータを読み込む */
	if (info.Bps() == 8) { d[0] = *data++; datasize--; }
	else /* info.Bps() == 16 */ { d[0] = read_little_endian_short(data); data += 2; datasize -= 2; }
	if (info.Channels() == 2) {
		if (info.Bps() == 8) { d[1] = *data++; datasize--; }
		else /* info.Bps() == 16 */ { d[1] = read_little_endian_short(data); data += 2; datasize -= 2; }
	}
	int dsize = outdatasize / (info.Bps() / 8);
	int flip_flag = 0; /* stereo 用 */
	int runlength = 0;
	for (i = 0; i < dsize; i++) {
		if (data >= dataend) break;
		if (runlength == 0) { // コピーループ中でないならデータ読み込み
			int type = getbits(data, shift, 3);
			/* type により分岐：0, 1-6, 7 */
			if (type == 7) {
				/* 7 : 大きな差分 */
				/* RunLength() 有効時（CompLevel==5, 音声ファイル) では無効 */
				if (getbits(data, shift, 1) == 1) {
					d[flip_flag] = 0; /* 未使用 */
				}
				else {
					int BITS, SHIFT;
					if (info.CompLevel() >= 3) {
						BITS = 8;
						SHIFT = 9;
					}
					else {
						BITS = 8 - info.CompLevel();
						SHIFT = 2 + 7 + info.CompLevel();
					}
					const int MASK1 = (1 << (BITS - 1));
					const int MASK2 = (1 << (BITS - 1)) - 1;
					int b = getbits(data, shift, BITS);
					if (b&MASK1)
						d[flip_flag] -= (b&MASK2) << SHIFT;
					else
						d[flip_flag] += (b&MASK2) << SHIFT;
				}
			}
			else if (type != 0) {
				/* 1-6 : 通常の差分 */
				int BITS, SHIFT;
				if (info.CompLevel() >= 3) {
					BITS = info.CompLevel() + 3;
					SHIFT = 1 + type;
				}
				else {
					BITS = 5 - info.CompLevel();
					SHIFT = 2 + type + info.CompLevel();
				}
				const int MASK1 = (1 << (BITS - 1));
				const int MASK2 = (1 << (BITS - 1)) - 1;
				int b = getbits(data, shift, BITS);
				if (b&MASK1)
					d[flip_flag] -= (b&MASK2) << SHIFT;
				else
					d[flip_flag] += (b&MASK2) << SHIFT;
			}
			else { /* type == 0 */
				/* ランレングス圧縮なしの場合はなにもしない */
				if (info.UseRunLength() == true) {
					/* ランレングス圧縮ありの場合 */
					runlength = getbits(data, shift, 1);
					if (runlength == 1) {
						runlength = getbits(data, shift, 2);
						if (runlength == 3) {
							runlength = getbits(data, shift, 8);
						}
					}
				}
			}
		}
		else {
			runlength--;
		}
		if (info.Bps() == 8) {
			*outdata++ = d[flip_flag];
		}
		else {
			write_little_endian_short(outdata, d[flip_flag]);
			outdata += 2;
		}
		if (info.Channels() == 2) flip_flag ^= 1; /* channel 切り替え */
	}
	return;
};

class NWAData {
public:
	int channels;
	int bps; /* bits per sample */
	int freq; /* samples per second */
private:
	int complevel; /* compression level */
	int use_runlength; /* run length encoding */
public:
	int blocks; /* block count */
	int datasize; /* all data size */
private:
	int compdatasize; /* compressed data size */
	int samplecount; /* all samples */
	int blocksize; /* samples per block */
	int restsize; /* samples of the last block */
	int dummy2; /* ? : 0x89 */
	int curblock;
	int* offsets;
	int offset_start;
	int filesize;
	char* tmpdata;
public:
	void ReadHeader(NtFileDisk& in, int file_size = -1);
	int CheckHeader(void); /* false: invalid true: valid */
	NWAData(void) {
		offsets = 0;
		tmpdata = 0;
	}
	~NWAData(void) {
		if (offsets) delete[] offsets;
		if (tmpdata) delete[] tmpdata;
	}
	int BlockLength(void) {
		if (complevel != -1) {
			if (offsets == 0) return false;
			if (tmpdata == 0) return false;
		}
		return blocksize * (bps / 8);
	}
	/* data は BlockLength 以上の長さを持つこと
	** 返り値は作成したデータの長さ。終了時は 0。
	** エラー時は -1
	*/
	int Decode(NtFileDisk& in, char* data, int& skip_count);
	void Rewind(NtFileDisk& in);
};

void NWAData::ReadHeader(NtFileDisk& in, int _file_size) {
	char header[0x2c];
	struct stat sb;
	int i;
	if (offsets) delete[] offsets;
	if (tmpdata) delete[] tmpdata;
	offsets = 0;
	tmpdata = 0;
	filesize = 0;
	offset_start = in.GetCurrentPos();
	if (offset_start == -1) offset_start = 0;
	if (_file_size != -1) filesize = _file_size;
	curblock = -1;
	/* header 読み込み */
	in.Read(header, 0x2c);

	channels = read_little_endian_short(header + 0x00);
	bps = read_little_endian_short(header + 0x02);
	freq = read_little_endian_int(header + 0x04);
	complevel = read_little_endian_int(header + 0x08);
	use_runlength = read_little_endian_int(header + 0x0c);
	blocks = read_little_endian_int(header + 0x10);
	datasize = read_little_endian_int(header + 0x14);
	compdatasize = read_little_endian_int(header + 0x18);
	samplecount = read_little_endian_int(header + 0x1c);
	blocksize = read_little_endian_int(header + 0x20);
	restsize = read_little_endian_int(header + 0x24);
	dummy2 = read_little_endian_int(header + 0x28);
	if (complevel == -1) {	/* 無圧縮rawデータ */
		/* 適当に決め打ちする */
		blocksize = 65536;
		restsize = (datasize % (blocksize * (bps / 8))) / (bps / 8);
		blocks = datasize / (blocksize * (bps / 8)) + (restsize > 0 ? 1 : 0);
	}
	if (blocks <= 0 || blocks > 1000000) {
		return;
	}
	/* regular file なら filesize 読み込み */
	if (filesize == 0) {
		int pos = in.GetCurrentPos();
		filesize = in.GetSize32();
		if (pos + blocks * 4 >= filesize) {
			return;
		}
	}
	if (complevel == -1) return;
	/* offset index 読み込み */
	offsets = new int[blocks];
	in.Read(offsets, blocks * 4);
	for (i = 0; i < blocks; i++) {
		offsets[i] = read_little_endian_int((char*)(offsets + i));
	}
}
void NWAData::Rewind(NtFileDisk& in) {
	curblock = -1;
	in.Seek(0x2c, FILE_BEGIN);
	if (offsets) in.Seek(blocks * 4, FILE_CURRENT);
}
int NWAData::CheckHeader(void)
{
	if (complevel != -1 && offsets == 0) return false;
	/* データそのもののチェック */
	if (channels != 1 && channels != 2) {
		return false;
	}
	if (bps != 8 && bps != 16) {
		return false;
	}
	if (complevel == -1) {
		int byps = bps / 8; /* bytes per sample */
		if (datasize != samplecount*byps) {
			return false;
		}
		if (samplecount != (blocks - 1)*blocksize + restsize) {
			return false;
		}
		else
			return true;
	}
	if (complevel < 0 || complevel > 5) {
		return false;
	}
	/* 整合性チェック */
	if (filesize != 0 && filesize != compdatasize) {
		return false;
	}
	if (offsets[blocks - 1] >= compdatasize) {
		return false;
	}
	int byps = bps / 8; /* bytes per sample */
	if (datasize != samplecount*byps) {
		return false;
	}
	if (samplecount != (blocks - 1)*blocksize + restsize) {
		return false;
	}
	tmpdata = new char[blocksize*byps * 2]; /* これ以上の大きさはないだろう、、、 */
	return true;
}

class NWAInfo_sw2 {
public:
	int Channels(void) const{ return 2; }
	int Bps(void) const { return 16; }
	int CompLevel(void) const { return 2; }
	int UseRunLength(void) const { return false; }
};
int NWAData::Decode(NtFileDisk& in, char* data, int& skip_count) {
	if (complevel == -1) {		/* 無圧縮時の処理 */
		if (curblock == -1) {
			/* 最初のブロックなら、wave header 出力 */
			memcpy(data, make_wavheader(datasize, channels, bps, freq), 0x2c);
			curblock++;
			in.Seek(offset_start + 0x2c, FILE_BEGIN);
			return 0x2c;
		}
		if (skip_count > blocksize / channels) {
			skip_count -= blocksize / channels;
			in.Seek(blocksize*(bps / 8), FILE_CURRENT);
			curblock++;
			return -2;
		}
		if (curblock < blocks) {
			int readsize = blocksize;
			if (skip_count) {
				in.Seek(skip_count*channels*(bps / 8), FILE_CURRENT);
				readsize -= skip_count * channels;
				skip_count = 0;
			}
			LARGE_INTEGER err;
			err.QuadPart = 0;
			in.Read(data, readsize * (bps / 8), &err);
			curblock++;
			return err.LowPart;
		}
		return -1;
	}
	if (offsets == 0 || tmpdata == 0) return -1;
	if (blocks == curblock) return 0;
	if (curblock == -1) {
		/* 最初のブロックなら、wave header 出力 */
		memcpy(data, make_wavheader(datasize, channels, bps, freq), 0x2c);
		curblock++;
		return 0x2c;
	}
	/* 今回読み込む／デコードするデータの大きさを得る */
	int curblocksize, curcompsize;
	if (curblock != blocks - 1) {
		curblocksize = blocksize * (bps / 8);
		curcompsize = offsets[curblock + 1] - offsets[curblock];
		if (curblocksize >= blocksize*(bps / 8) * 2) return -1; // Fatal error
	}
	else {
		curblocksize = restsize * (bps / 8);
		curcompsize = blocksize*(bps / 8) * 2;
	}
	if (skip_count > blocksize / channels) {
		skip_count -= blocksize / channels;
		in.Seek(curcompsize, FILE_CURRENT);
		curblock++;
		return -2;
	}
	/* データ読み込み */
	in.Read(tmpdata, curcompsize);
	/* 展開 */
	if (channels == 2 && bps == 16 && complevel == 2) {
		NWAInfo_sw2 info;
		NWADecode(info, tmpdata, data, curcompsize, curblocksize);
	}
	else {
		NWAInfo info(channels, bps, complevel, use_runlength);
		NWADecode(info, tmpdata, data, curcompsize, curblocksize);
	}
	int retsize = curblocksize;
	if (skip_count) {
		int skip_c = skip_count * channels * (bps / 8);
		retsize -= skip_c;
		memmove(data, data + skip_c, skip_c);
		skip_count = 0;
	}
	curblock++;
	return retsize;
}


void ConvertNwaToWav(NtFileDisk& in, StreamWriter& out, int skip_count, int in_size)
{
	NWAData h;
	h.ReadHeader(in, in_size);
	h.CheckHeader();
	int bs = h.BlockLength();
	char* d = new char[bs];
	int err;
	while ((err = h.Decode(in, d, skip_count)) != 0)
	{
		if (err == -1) break;
		if (err == -2) continue;
		out.Write(d, err);
	}
	return;
}
