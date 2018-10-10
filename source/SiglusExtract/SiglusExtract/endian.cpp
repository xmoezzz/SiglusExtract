// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2007 Elliot Glaysher
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//
// -----------------------------------------------------------------------

#include "endian.hpp"

// -----------------------------------------------------------------------

/**
* @file   endian.cc
* @author Jagarl
* @author Elliot Glaysher
* @date   Sun Dec 30 08:58:09 2007
*
* @brief  This file is common code factored out from file.cc and
*         nwatowav.cc.
*
*
*/

// -----------------------------------------------------------------------

int read_little_endian_int(const char* buf) {
	const unsigned char *p = (const unsigned char *)buf;
	return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

// -----------------------------------------------------------------------

int read_little_endian_short(const char* buf) {
	const unsigned char *p = (const unsigned char *)buf;
	return (p[1] << 8) | p[0];
}

// -----------------------------------------------------------------------

int write_little_endian_int(char* buf, int number) {
	int c = read_little_endian_int(buf);
	unsigned char *p = (unsigned char *)buf;
	unsigned int unum = (unsigned int)number;
	p[0] = unum & 255;
	unum >>= 8;
	p[1] = unum & 255;
	unum >>= 8;
	p[2] = unum & 255;
	unum >>= 8;
	p[3] = unum & 255;
	return c;
}

// -----------------------------------------------------------------------

int write_little_endian_short(char* buf, int number) {
	int c = read_little_endian_short(buf);
	unsigned char *p = (unsigned char *)buf;
	unsigned int unum = (unsigned int)number;
	p[0] = unum & 255;
	unum >>= 8;
	p[1] = unum & 255;
	return c;
}
