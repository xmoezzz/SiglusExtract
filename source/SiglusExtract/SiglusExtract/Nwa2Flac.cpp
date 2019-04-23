#include "StreamWriter.h"
#include "NwaConverter.h"
#include <my.h>
#include <string.h>


#include "share/compat.h"
#include "FLAC/metadata.h"
#include "FLAC/stream_encoder.h"
#pragma comment(lib, "..\\Release\\libFLAC_static.lib")
#pragma comment(lib, "..\\Release\\libogg_static.lib")


#define READSIZE 1024

static FLAC__StreamEncoderWriteStatus
write_cb(const FLAC__StreamEncoder *env, const FLAC__byte *buf, size_t nbytes,
unsigned samples, unsigned current_frame, void *ctx)
{
	StreamWriter* writer = (StreamWriter*)ctx;
	writer->Write((PBYTE)buf, nbytes);
	return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

FLAC__StreamEncoderSeekStatus seek_cb(const FLAC__StreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *ctx)
{
	StreamWriter* writer = (StreamWriter*)ctx;
	writer->Seek((ULONG)absolute_byte_offset);
	return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}


FLAC__StreamEncoderTellStatus tell_cb(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *ctx)
{
	StreamWriter* writer = (StreamWriter*)ctx;
	*absolute_byte_offset = writer->Tell();
	return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
}

FLAC__StreamEncoderReadStatus read_cb(const FLAC__StreamEncoder *encoder, FLAC__byte buffer[], size_t *bytes, void *ctx)
{
	StreamWriter* writer = (StreamWriter*)ctx;
	if (*bytes > 0)
	{
		ULONG ret = writer->Read(buffer, *bytes);
		*bytes = ret;
		if (ret == 0)
			return FLAC__STREAM_ENCODER_READ_STATUS_END_OF_STREAM;

		return FLAC__STREAM_ENCODER_READ_STATUS_CONTINUE;
	}
	else
		return FLAC__STREAM_ENCODER_READ_STATUS_ABORT;

}

void ConvertNwaToFlacInternal(NtFileMemory& in, StreamWriter& out)
{
	FLAC__bool ok = true;
	FLAC__StreamEncoder *encoder = 0;
	FLAC__StreamEncoderInitStatus init_status;
	FLAC__StreamMetadata *metadata[2];
	FLAC__StreamMetadata_VorbisComment_Entry entry;
	unsigned sample_rate = 0;
	unsigned channels = 0;
	unsigned bps = 0;

	unsigned total_samples = 0;

	FLAC__byte buffer[READSIZE/*samples*/ * 2/*bytes_per_sample*/ * 2/*channels*/]; /* we read the WAVE data into here */
	FLAC__int32 pcm[READSIZE/*samples*/ * 2/*channels*/];

	/* read wav header and validate it */
	in.Read(buffer, 44);
	if (
		memcmp(buffer, "RIFF", 4) ||
		memcmp(buffer + 8, "WAVEfmt \020\000\000\000\001\000\002\000", 16) ||
		memcmp(buffer + 32, "\004\000\020\000data", 8)
		) {
		return;
	}
	sample_rate = ((((((unsigned)buffer[27] << 8) | buffer[26]) << 8) | buffer[25]) << 8) | buffer[24];
	channels = 2;
	bps = 16;
	total_samples = (((((((unsigned)buffer[43] << 8) | buffer[42]) << 8) | buffer[41]) << 8) | buffer[40]) / 4;

	/* allocate the encoder */
	if ((encoder = FLAC__stream_encoder_new()) == NULL) {
		return;
	}

	ok &= FLAC__stream_encoder_set_verify(encoder, true);
	ok &= FLAC__stream_encoder_set_compression_level(encoder, 5);
	ok &= FLAC__stream_encoder_set_channels(encoder, channels);
	ok &= FLAC__stream_encoder_set_bits_per_sample(encoder, bps);
	ok &= FLAC__stream_encoder_set_sample_rate(encoder, sample_rate);
	ok &= FLAC__stream_encoder_set_total_samples_estimate(encoder, total_samples);

	/* now add some metadata; we'll add some tags and a padding block */
	if (ok) {
		if (
			(metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) == NULL ||
			(metadata[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL ||
			/* there are many tag (vorbiscomment) functions but these are convenient for this particular use: */
			!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ARTIST", "Unknown Artist") ||
			!FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, /*copy=*/false) || /* copy=false: let metadata object take control of entry's allocated string */
			!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "YEAR", "2017") ||
			!FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, /*copy=*/false)
			) {
			ok = false;
		}

		metadata[1]->length = 1234; /* set the padding length */

		ok = FLAC__stream_encoder_set_metadata(encoder, metadata, 2);
	}

	/* initialize encoder */
	if (ok) {
		init_status = FLAC__stream_encoder_init_ogg_stream(encoder, read_cb, write_cb, seek_cb, tell_cb, NULL, &out);
		if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
			ok = false;
		}
	}

	/* read blocks of samples from WAVE file and feed to encoder */
	if (ok) 
	{
		size_t left = (size_t)total_samples;
		while (ok && left) 
		{
			size_t need = (left>READSIZE ? (size_t)READSIZE : (size_t)left);
			LARGE_INTEGER ReadSize;
			ReadSize.QuadPart = 0;
			in.Read(buffer, channels*(bps / 8) * need, &ReadSize);
			if (ReadSize.LowPart != need * (channels*(bps / 8))) {
				ok = false;
			}
			else {
				/* convert the packed little-endian 16-bit PCM samples from WAVE into an interleaved FLAC__int32 buffer for libFLAC */
				size_t i;
				for (i = 0; i < need*channels; i++) {
					/* inefficient but simple and works on big- or little-endian machines */
					pcm[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)buffer[2 * i + 1] << 8) | (FLAC__int16)buffer[2 * i]);
				}
				/* feed samples to encoder */
				ok = FLAC__stream_encoder_process_interleaved(encoder, pcm, need);
			}
			left -= need;
		}
	}

	ok &= FLAC__stream_encoder_finish(encoder);

	/* now that encoding is finished, the metadata can be freed */
	FLAC__metadata_object_delete(metadata[0]);
	FLAC__metadata_object_delete(metadata[1]);

	FLAC__stream_encoder_delete(encoder);
}


void ConvertNwaToFlacInternalV2(NtFileMemory& in, LPCWSTR FileName)
{
	FLAC__bool ok = true;
	FLAC__StreamEncoder *encoder = 0;
	FLAC__StreamEncoderInitStatus init_status;
	FLAC__StreamMetadata *metadata[2];
	FLAC__StreamMetadata_VorbisComment_Entry entry;
	unsigned sample_rate = 0;
	unsigned channels = 0;
	unsigned bps = 0;

	unsigned total_samples = 0;

	FLAC__byte buffer[READSIZE/*samples*/ * 2/*bytes_per_sample*/ * 2/*channels*/]; /* we read the WAVE data into here */
	FLAC__int32 pcm[READSIZE/*samples*/ * 2/*channels*/];

	/* read wav header and validate it */
	in.Read(buffer, 44);
	if (
		memcmp(buffer, "RIFF", 4) ||
		memcmp(buffer + 8, "WAVEfmt \020\000\000\000\001\000\002\000", 16) ||
		memcmp(buffer + 32, "\004\000\020\000data", 8)
		) {
		return;
	}
	sample_rate = ((((((unsigned)buffer[27] << 8) | buffer[26]) << 8) | buffer[25]) << 8) | buffer[24];
	channels = 2;
	bps = 16;
	total_samples = (((((((unsigned)buffer[43] << 8) | buffer[42]) << 8) | buffer[41]) << 8) | buffer[40]) / 4;

	/* allocate the encoder */
	if ((encoder = FLAC__stream_encoder_new()) == NULL) {
		return;
	}

	ok &= FLAC__stream_encoder_set_verify(encoder, true);
	ok &= FLAC__stream_encoder_set_compression_level(encoder, 5);
	ok &= FLAC__stream_encoder_set_channels(encoder, channels);
	ok &= FLAC__stream_encoder_set_bits_per_sample(encoder, bps);
	ok &= FLAC__stream_encoder_set_sample_rate(encoder, sample_rate);
	ok &= FLAC__stream_encoder_set_total_samples_estimate(encoder, total_samples);

	/* now add some metadata; we'll add some tags and a padding block */
	if (ok) {
		if (
			(metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) == NULL ||
			(metadata[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL ||
			/* there are many tag (vorbiscomment) functions but these are convenient for this particular use: */
			!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ARTIST", "Unknown Artist") ||
			!FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, /*copy=*/false) || /* copy=false: let metadata object take control of entry's allocated string */
			!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "YEAR", "2017") ||
			!FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, /*copy=*/false)
			) {
			ok = false;
		}

		metadata[1]->length = 1234; /* set the padding length */

		ok = FLAC__stream_encoder_set_metadata(encoder, metadata, 2);
	}

	/* initialize encoder */
	if (ok) {
		FILE* fout = _wfopen(FileName, L"w+b");
		init_status = FLAC__stream_encoder_init_FILE(encoder, fout, NULL, /*client_data=*/NULL);
		if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
			ok = false;
		}
	}

	/* read blocks of samples from WAVE file and feed to encoder */
	if (ok)
	{
		size_t left = (size_t)total_samples;
		while (ok && left)
		{
			size_t need = (left>READSIZE ? (size_t)READSIZE : (size_t)left);
			LARGE_INTEGER ReadSize;
			ReadSize.QuadPart = 0;
			in.Read(buffer, channels*(bps / 8) * need, &ReadSize);
			if (ReadSize.LowPart != need * (channels*(bps / 8))) {
				ok = false;
			}
			else {
				/* convert the packed little-endian 16-bit PCM samples from WAVE into an interleaved FLAC__int32 buffer for libFLAC */
				size_t i;
				for (i = 0; i < need*channels; i++) {
					/* inefficient but simple and works on big- or little-endian machines */
					pcm[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)buffer[2 * i + 1] << 8) | (FLAC__int16)buffer[2 * i]);
				}
				/* feed samples to encoder */
				ok = FLAC__stream_encoder_process_interleaved(encoder, pcm, need);
			}
			left -= need;
		}
	}

	ok &= FLAC__stream_encoder_finish(encoder);

	/* now that encoding is finished, the metadata can be freed */
	FLAC__metadata_object_delete(metadata[0]);
	FLAC__metadata_object_delete(metadata[1]);

	FLAC__stream_encoder_delete(encoder);
}

void ConvertNwaToFlac(NtFileDisk& in, StreamWriter& out)
{
	StreamWriter WavStream;
	NtFileMemory WavMemory;

	ConvertNwaToWav(in, WavStream);

	WavMemory.Open(WavStream.GetBuffer(), WavStream.GetSize());
	ConvertNwaToFlacInternal(WavMemory, out);
}

void ConvertNwaToFlac(NtFileDisk& in, LPCWSTR FileName)
{
	StreamWriter WavStream;
	NtFileMemory WavMemory;

	ConvertNwaToWav(in, WavStream);

	WavMemory.Open(WavStream.GetBuffer(), WavStream.GetSize());
	ConvertNwaToFlacInternalV2(WavMemory, FileName);
}
