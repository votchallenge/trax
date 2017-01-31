/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

#ifndef BASE64_H
#define BASE64_H

int base64decodelen(const char *bufcoded);

int base64decode(unsigned char *bufplain, const char *bufcoded);

int base64encodelen(int len);

int base64encode(char *encoded, const unsigned char *string, int len);

#endif 
