#ifndef UTIL_H
#define UTIL_H

void packed_EAN_to_BCD(unsigned char *ean, unsigned char *bcd);
void packed_EAN_to_BCD_with_csum(unsigned char *ean, unsigned char *bcd);
unsigned int calculateCRC32(void *buf, unsigned int bufsiz);
#endif
