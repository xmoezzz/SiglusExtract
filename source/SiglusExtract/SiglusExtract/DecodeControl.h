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

typedef struct DecodeControl
{
	G00Decode G00Flag;
	NWADecode NWAFlag;
	OGGDecode OGGFlag;
	OGVDecode OGVFlag;
	SSDecode  SSDecode;
	BYTE      PrivateKey[16];
	BOOL      PckNeedKey;
	BOOL      DatNeedKey;
	
	DecodeControl()
	{
		RtlZeroMemory(PrivateKey, sizeof(PrivateKey));
	}
	
	BOOL IsValidKey()
	{
		DWORD Count;

		Count = 0;
		for (SIZE_T i = 0; i < sizeof(PrivateKey); i++)
			if (PrivateKey[i] == 0)
				Count++;

		return Count != sizeof(PrivateKey);
	}

}DecodeControl, *PDecodeControl;