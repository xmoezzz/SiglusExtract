#pragma once

#include <vector>
#include "my.h"

struct StreamWriter
{
	std::vector<BYTE> m_Stream;
	ULONG             m_Offset;

	StreamWriter()
	{
		m_Offset = 0;
	}

	ULONG GetSize()
	{
		return m_Stream.size();
	}

	PBYTE GetBuffer()
	{
		return &m_Stream[0];
	}

	ULONG Write(PVOID Buffer, ULONG Size)
	{
		if (m_Offset + Size > m_Stream.size())
		{
			ULONG Len = m_Offset + Size - m_Stream.size();
			for (ULONG i = 0; i < Len; i++)
				m_Stream.push_back(NULL);
		}
		for (ULONG i = 0; i < Size; i++)
			m_Stream.push_back(((PBYTE)Buffer)[i]);

		RtlCopyMemory(&m_Stream[m_Offset], Buffer, Size);
		m_Offset += Size;

		return Size;
	}

	ULONG Read(PVOID Buffer, ULONG Size)
	{
		ULONG Result;

		if (m_Offset + Size > m_Stream.size())
			Result = m_Stream.size() - m_Offset;
		else
			Result = Size;

		RtlCopyMemory(Buffer, &m_Stream[m_Offset], Size);
		m_Offset += Result;
		return Result;
	}

	ULONG Tell()
	{
		return m_Offset;
	}

	BOOL Seek(ULONG AbsolutelyOffset)
	{
		if (AbsolutelyOffset > m_Stream.size())
			return FALSE;

		m_Offset = AbsolutelyOffset;
		return TRUE;
	}
};
