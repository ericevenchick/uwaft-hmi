#include "asm/div64.h"

#include "util.h"

#if 0
/****************************************************************************/
char *itoa (int i, char *buf, int base)
{
  int len = 0;
  int z   = i;
  char *x = buf;

  if (buf) {
    strcpy (buf, "");
  }

  if (!buf || (base < 2) || (base > 16)) {
    return NULL;
  }

  do {
    z /= base;
    ++len;
  } while (z != 0);

  z = i;
  x += len;
  *x = '\0';
  --x;

  do {
    int m = z % base;
    if ((m >= 0) && (m <= 9)) {
      *x = '0' + m;
    } else {
      *x = 'a' + m;
    }
    z /= base;
    --x;
  } while (/*(z != 0) &&*/ (x >= buf));

  return buf;
} /* itoa */
#endif


/****************************************************************************/
void packed_EAN_to_BCD(unsigned char *ean, unsigned char *bcd)
{
  long long ean64;
  int i;

  //
  // "ean" points to a 40 bit number (LSB first) which is the EAN code.
  // The ean code is followed by the check digit in a separate byte.
  // This is the standard format for DRVcans and LAPcan.
  //
  // "bcd" points to a buffer which will contain the EAN number as
  // a BCD string, LSB first. The buffer must be at least 8 bytes.
  //
  // E.g. 733-0130-00122-0 would be (hex)
  // 20 12 00 30 01 33 07 00
  //
  ean64 = 0;
  for (i = 0; i < 5; i++) {
    ean64 <<= 8;
    ean64 += ean[4 - i];
  }
  ean64 *= 10;
  ean64 += ean[5];

  // Store the EAN number as a BCD string.
  for (i = 0; i < 8; i++) {
    unsigned char c1, c2;
#if 0
    c1 = (unsigned char)(ean64 % 10);
    ean64 /= 10;
    c2 = (unsigned char)(ean64 % 10);
    ean64 /= 10;
#else
    c1 = do_div(ean64, 10);
    c2 = do_div(ean64, 10);
#endif
    *bcd = c1 + (c2 << 4);
    bcd++;
  }
}


/****************************************************************************/
void packed_EAN_to_BCD_with_csum(unsigned char *ean, unsigned char *bcd)
{
  long long ean64, tmp;
  int i;
  unsigned int csum;

  //
  // "ean" points to a 40 bit number (LSB first) which is the EAN code.
  // The ean code is NOT followed by a check digit.
  // This is the standard format for DRVcans and LAPcan.
  //
  // "bcd" points to a buffer which will contain the EAN number as
  // a BCD string, LSB first. The buffer must be at least 8 bytes.
  //
  // E.g. 733-0130-00122-0 would be (hex)
  // 20 12 00 30 01 33 07 00
  //
  ean64 = 0;
  for (i = 0; i < 5; i++) {
    ean64 <<= 8;
    ean64 += ean[4 - i];
  }

  // Calculate checksum
  tmp = ean64;
  csum = 0;
  for (i = 0; i < 12; i++) {
    unsigned int x;
    
#if 0
    x = (unsigned int)(tmp % 10);
    tmp /= 10;
#else
    x = do_div(tmp, 10);
#endif
    
    if (i % 2 == 0) {
      csum += 3 * x;
    } else {
      csum += x;
    }
  }
  csum = 10 - csum % 10;
  if (csum == 10) {
    csum = 0;
  }

  ean64 *= 10;
  ean64 += csum;

  // Store the EAN number as a BCD string.
  for (i = 0; i < 8; i++) {
    unsigned char c1, c2;
#if 0
    c1 = (unsigned char)(ean64 % 10);
    ean64 /= 10;
    c2 = (unsigned char)(ean64 % 10);
    ean64 /= 10;
#else
    c1 = do_div(ean64, 10);
    c2 = do_div(ean64, 10);
#endif
    *bcd = c1 + (c2 << 4);
    bcd++;
  }
}


/****************************************************************************/
/*
** CRC32 routine. It is recreating its table for each calculation.
** This is by design - it is not intended to be used frequently so we
** perfer to save some memory instead.
**
** The following C code (by Rob Warnock <rpw3@sgi.com>) does CRC-32 in
** BigEndian/BigEndian byte/bit order.  That is, the data is sent most
** significant byte first, and each of the bits within a byte is sent most
** significant bit first, as in FDDI. You will need to twiddle with it to do
** Ethernet CRC, i.e., BigEndian/LittleEndian byte/bit order. [Left as an
** exercise for the reader.]
**
** The CRCs this code generates agree with the vendor-supplied Verilog models
** of several of the popular FDDI "MAC" chips.
*/
#define CRC32_POLY    0x04c11db7L     /* AUTODIN II, Ethernet, & FDDI */

unsigned int calculateCRC32(void *buf, unsigned int bufsiz)
{
  unsigned char *p;
  unsigned int crc;
  unsigned int crc32_table[256];
  int i, j;

  for (i = 0; i < 256; ++i) {
    unsigned int c;
    for (c = i << 24, j = 8; j > 0; --j) {
      c = c & 0x80000000L ? (c << 1) ^ CRC32_POLY : (c << 1);
    }
    crc32_table[i] = c;
  }

  crc = 0xffffffffL;      /* preload shift register, per CRC-32 spec */
  for (p = buf; bufsiz > 0; ++p, --bufsiz) {
    crc = (crc << 8) ^ crc32_table[(crc >> 24) ^ *(unsigned char *)p];
  }
  return ~crc;            /* transmit complement, per CRC-32 spec */
}
