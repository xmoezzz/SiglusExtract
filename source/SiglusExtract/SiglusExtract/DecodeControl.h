#pragma once

typedef enum G00Decode
{
	G00_BMP,
	G00_PNG,
	G00_JPG
}G00Decode;

typedef enum NWADecode
{
	NWA_WAV,
	NWA_FLAC,
	NWA_VORBIS
}NWADecode;

typedef enum OGGDecode
{
	OGG_RAW,
	OGG_DECODE
}OGGDecode;

typedef enum OGVDecode
{
	OGV_DECODE,
	OGV_PNG
}OGVDecode;

typedef enum SSDecode
{
	SS_V1,
	SS_V2
}SSDecode;

typedef struct _DecodeControl
{
	G00Decode G00Flag;
	NWADecode NWAFlag;
	OGGDecode OGGFlag;
	OGVDecode OGVFlag;
	SSDecode  SSDecode;
	unsigned char PrivateKey[16];
	BOOL      PckNeedKey;
	BOOL      DatNeedKey;
}DecodeControl, *PDecodeControl;