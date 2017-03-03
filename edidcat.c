/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Gaël PORTAY <gael.portay@gmail.com>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#else
const char VERSION[] = __DATE__ " " __TIME__;
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>

#include <byteswap.h>
#include <arpa/inet.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
# define ntohll(x)	__bswap_64(x)
# define htonll(x)	__bswap_64(x)
#else
# define ntohll(x)	(x)
# define htonll(x)	(x)
#endif

#include "hexdump.h"

#define __strncmp(s1, s2) strncmp(s1, s2, sizeof(s2))

struct options_t {
	const char *prefix;
	const char *export;
	int debug;
};

static struct options_t options;

#define PREFIX	((options.prefix) ? options.prefix : "")
#define EXPORT	((options.export) ? options.export : "")
#define DEBUG	(options.debug)

#define debug(fmt, ...)		if (DEBUG) fprintf(stderr, fmt, ##__VA_ARGS__)
#define hexdebug(a, b, s)	if (DEBUG) fhexdump(stderr, a, b, s)

/*
 * According to wikipedia
 *
 * https://en.wikipedia.org/wiki/Extended_Display_Identification_Data
 */

/*
 * EDID Other Monitor Descriptors
 *
 * Bytes 	Description
 * 0–1		Zero, indicates not a detailed timing descriptor
 * 2		Zero
 * 3		Zero
 * 4		Descriptor type.
 *		FA–FF currently defined. 00–0F reserved for vendors.
 * 5		Zero
 * 6-17 	Defined by descriptor type.
 *		If text, code page 437 text, terminated (if less than 13 bytes)
 *		with LF and padded with SP.
 */

/*
 * 0xFF: Monitor serial number (text)
 * 0xFE: Unspecified text (text)
 * 0xFD: Monitor range limits. 6- or 13-byte binary descriptor.
 * 0xFC: Monitor name (text), padded with 0A 20 20.
 * 0xFB: Additional white point data.
 *       2× 5-byte descriptors, padded with 0A 20 20.
 * 0xFA: Additional standard timing identifiers. 6× 2-byte descriptors, padded
 *       with 0A.
 */

#define LF		0x0A
#define SP		0x20
#define DESCRIPTOR_TEXT	0xFE
#define DESCRIPTOR_NAME	0xFC

struct descriptor {
	uint8_t is_timing;
	uint8_t unused[2];
	uint8_t type;
	uint8_t unused_2[1];
	uint8_t data[13];
} __attribute__((packed));

/*
 * Bytes	Description
 * 0–19		Header information
 *
 * 0–7		Fixed header pattern: 00 FF FF FF FF FF FF 00
 * 8–9		Manufacturer ID.
 * 		These IDs are assigned by Microsoft, they are PNP IDs "00001=A”;
 *		“00010=B”; ... “11010=Z”.
 *		Bit 7 (at address 08h) is 0,
 *		the first character (letter) is located at bits 6 → 2
 *		(at address 08h),
 *		the second character (letter) is located at bits 1 & 0
 *		(at address 08h) and bits 7 → 5 (at address 09h), and
 *		the third character (letter) is located at bits 4 → 0
 *		(at address 09h).
 *  Bit 15	(Reserved, always 0)
 *  Bits 14–10 	First letter of manufacturer ID
 *		(byte 8, bits 6–2)
 *  Bits 9–5 	Second letter of manufacturer ID
 *		(byte 8, bit 1 through byte 9 bit 5)
 *  Bits 4–0 	Third letter of manufacturer ID
 *		(byte 9 bits 4–0)
 * 10–11 	Manufacturer product code. 16-bit number, little-endian.
 * 12–15 	Serial number. 32 bits, little endian.
 * 16		Week of manufacture.
 *		Week numbering is not consistent between manufacturers.
 * 17		Year of manufacture, less 1990. (1990–2245).
 *		If week=255, it is the model year instead.
 * 18		EDID version, usually 1 (for 1.3)
 * 19		EDID revision, usually 3 (for 1.3)
 */

struct header {
	uint64_t magic;
	uint16_t id;
	uint16_t code;
	uint32_t serial;
	uint8_t	week;
	uint8_t	year;
	uint8_t	edid_version;
	uint8_t	edid_revision;
	uint8_t unused[34];
	struct descriptor descriptor_1;
	struct descriptor descriptor_2;
	struct descriptor descriptor_3;
	struct descriptor descriptor_4;
	uint8_t extentions;
	uint8_t checksum;
} __attribute__((packed));

#define MAGIC		ntohll(0x00FFFFFFFFFFFF00)

#define ID_BYTE_1(x)	(((x) & 0x7C00) >> 10)
#define ID_BYTE_2(x)	(((x) & 0x03E0) >>  5)
#define ID_BYTE_3(x)	( (x) & 0x001F       )

#define ID_CHAR_1(x)	(ID_BYTE_1((x)) + ('A' - 1))
#define ID_CHAR_2(x)	(ID_BYTE_2((x)) + ('A' - 1))
#define ID_CHAR_3(x)	(ID_BYTE_3((x)) + ('A' - 1))

int pr_descriptor(struct descriptor *descriptor)
{
	char buf[13+1]; /* +1 for NUL character */
	char *p;
	int i;

	/* Skip timing */
	if (descriptor->is_timing)
		return 0;

	/* Not a string type */
	if ((descriptor->type != DESCRIPTOR_NAME) &&
	    (descriptor->type != DESCRIPTOR_TEXT))
		return 0;

	/* Copy data */
	memcpy(buf, descriptor->data, sizeof(descriptor->data));
	buf[13] = 0;
	p = &buf[12];

	/* Remove SP padding if any */
	while ((p > buf) && (*p == SP))
		p--;
	if (*p == LF)
		*p = 0;

	/* Sanitize string */
	for (i = 0; i < 13; i++) {
		if (buf[i] == 0)
			break;

		/* Set non-printable character to _ */
		if ((buf[i] < 0x20) || (buf[i] >= 0x7F))
			buf[i] = '_';
	}

	if (descriptor->type == DESCRIPTOR_NAME)
		printf("%s%sNAME=\"%.13s\"\n", EXPORT, PREFIX, buf);
	else
		printf("%s%sTEXT=\"%.13s\"\n", EXPORT, PREFIX, buf);

	if (DEBUG && memcmp(buf, descriptor->data, 13))
		printf("%s%s__=\"%.13s\"\n", EXPORT, PREFIX, descriptor->data);

	return 0;
}

int pr_header(struct header *header)
{
	unsigned int i;
	uint8_t *data;
	uint32_t sum;

	/* Check magic */
	if (header->magic != MAGIC) {
		fprintf(stderr, "%lX: Invalid Magic-Number!\n",
		        header->magic);
		return -1;
	}

	/* Check sum */
	data = (uint8_t *)header;
	sum = 0;
	for (i = 0; i < sizeof(*header); i++)
		sum += data[i];
	if (sum % 256) {
		fprintf(stderr, "%u: Mismatch checksum!\n", sum);
		return -1;
	}

	/* Dump info */
	printf("%s%sID=\"%c%c%c\"\n", EXPORT, PREFIX,
	       ID_CHAR_1(__bswap_16(header->id)),
	       ID_CHAR_2(__bswap_16(header->id)),
	       ID_CHAR_3(__bswap_16(header->id)));
	printf("%s%sCODE=%04X\n", EXPORT, PREFIX, __bswap_16(header->code));
	printf("%s%sSERIAL=%04X\n", EXPORT, PREFIX, __bswap_16(header->serial));
	printf("%s%sWEEK=%u\n", EXPORT, PREFIX, header->week);
	printf("%s%sYEAR=%u\n", EXPORT, PREFIX, 1990 + header->year);
	printf("%s%sEDID_VERSION=%u\n", EXPORT, PREFIX, header->edid_version);
	printf("%s%sEDID_REVISION=%u\n", EXPORT, PREFIX, header->edid_revision);

	pr_descriptor(&header->descriptor_1);
	pr_descriptor(&header->descriptor_2);
	pr_descriptor(&header->descriptor_3);
	pr_descriptor(&header->descriptor_4);

	return header->extentions;
}

static inline const char *applet(const char *arg0)
{
	const char *applet = strrchr(arg0, '/');
	if (!applet)
		return arg0;

	return ++applet;
}

void usage(FILE * f, char * const arg0)
{
	fprintf(f, "Usage: %s [OPTIONS] [EDID] [EDID...]\n\n"
		   "Concatenate EDID(s) to standard output.\n\n"
		   "With no EDID, or when EDID is -, read standard input.\n\n"
		   "Options:\n"
		   " -p or --prefix STRING  Prefix variables.\n"
		   " -e or --export         Set export.\n"
		   " -D or --debug          Turn on debug messages.\n"
		   " -V or --version        Display the version.\n"
		   " -h or --help           Display this message.\n"
		   "", applet(arg0));
}

int parse_arguments(struct options_t *opts, int argc, char * const argv[])
{
	static const struct option long_options[] = {
		{ "prefix",  required_argument, NULL, 'p' },
		{ "export",  no_argument,       NULL, 'e' },
		{ "debug",   no_argument,       NULL, 'D' },
		{ "version", no_argument,       NULL, 'V' },
		{ "help",    no_argument,       NULL, 'h' },
		{ NULL,      no_argument,       NULL,  0  }
	};

	for (;;) {
		int index;
		int c = getopt_long(argc, argv, "p:eDVh", long_options, &index);
		if (c == -1)
			break;

		switch (c) {
		case 'p':
			opts->prefix = optarg;
			break;

		case 'e':
			opts->export = "export ";
			break;

		case 'D':
			DEBUG++;
			break;

		case 'V':
			printf("%s\n", VERSION);
			exit(EXIT_SUCCESS);
			break;

		case 'h':
			usage(stdout, argv[0]);
			exit(EXIT_SUCCESS);
			break;

		default:
		case '?':
			return -1;
		}
	}

	return optind;
}

int main(int argc, char * const argv[])
{
	unsigned char buf[128];
	ssize_t size = 0;
	ssize_t s;
	int ext = 0;
	int fd;
	int i;

	int argi = parse_arguments(&options, argc, argv);
	if (argi < 0) {
		fprintf(stderr, "Error: Invalid arguments!\n");
		exit(EXIT_FAILURE);
	}

	for (i = argi; i < argc; i++) {
		if (__strncmp(argv[i], "-") == 0) {
			fd = STDIN_FILENO;
		} else {
			fd = open(argv[i], O_RDONLY);
			if (fd == -1) {
				perror("open");
				exit(EXIT_FAILURE);
			}
		}

		for (;;) {
			s = read(fd, &buf[size], sizeof(buf) - size);
			if (s == -1) {
				perror("read");
				exit(EXIT_FAILURE);
			} else if (s == 0) {
				break;
			}

			size += s;
			if (size == sizeof(buf)) {
				hexdebug(ext, buf, size);
				if (!ext) {
					ext = pr_header((struct header *)buf);
					if (ext == -1) {
						hexdump(0, buf, size);
						exit(EXIT_FAILURE);
					}
				} else {
					ext--;
				}
				size = 0;
			}
		}

		if (fd != STDIN_FILENO)
			close(fd);
	}


	return EXIT_SUCCESS;
}
