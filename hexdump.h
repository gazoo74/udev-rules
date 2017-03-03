/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2017 Gaël PORTAY <gael.portay@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __HEXDUMP_H__
#define __HEXDUMP_H__

#include <stdio.h>

#define __min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

static int DUMPHEADER = 1;
static int DUMPFOOTER = 1;
static int DUMPADDR = 1;
static unsigned int ROWSIZE = 0x10;
static char EMPTYBYTE = ' ';

static void fhexdump_line(FILE *file, const void *buffer, unsigned int bufsize)
{
	const unsigned char *buf = (unsigned char *) buffer;
	unsigned int j;

	for (j = 0; j < __min(ROWSIZE, bufsize); j++) {
		fprintf(file, " %02x", buf[j]);
	}

	for (j = 0; j < __min(ROWSIZE-bufsize, ROWSIZE); j++) {
		fprintf(file, " %-2c", EMPTYBYTE);
	}

	fprintf(file, "\t");

	for (j = 0; j < __min(ROWSIZE, bufsize); j++) {
		if ((buf[j] < 0x20) || (buf[j] >= 0x7F))
			fprintf(file, ".");
		else
			fprintf(file, "%c", (char) buf[j]);
	}

	for (j = 0; j < __min(ROWSIZE-bufsize, ROWSIZE); j++) {
		fprintf(file, "%c", EMPTYBYTE);
	}
}

static void fhexdump(FILE *file, unsigned int address, const void *buffer,
		     unsigned int bufsize)
{
	const unsigned char *buf = (const unsigned char *) buffer;
	unsigned int r, row, size = 0;

	row = bufsize / ROWSIZE;
	if (bufsize % ROWSIZE)
		row++;

	if (DUMPHEADER) {
		fprintf(file, "%s:", "@address");
		for (r = 0; r < ROWSIZE; r ++)
			fprintf(file, " %02x", r);
		fprintf(file, "\n");
	}

	for (r = 0; r < row; r++) {
		unsigned int s = __min(bufsize - size, ROWSIZE);

		if (DUMPADDR)
			fprintf(file, "%08x:", address + size);

		fhexdump_line(file, buf, s);
		buf += s;
		size += s;
		fprintf(file, "\n");
	}

	if (DUMPFOOTER) {
		fprintf(file, "%08x\n", address + size);
	}
}

static void hexdump(unsigned int address, const void *buffer, unsigned int
		    bufsize)
{
	fhexdump(stdout, address, buffer, bufsize);
}

#endif /* __HEXDUMP_H__ */
