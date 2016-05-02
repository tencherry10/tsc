
#include "libts.h"

const char* ts_base64_enc(char **ret, char *data, size_t sz) {
  static char base64_enc_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                    'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                    '4', '5', '6', '7', '8', '9', '+', '/'};
  static int  mod_table[] = {0, 2, 1};
  size_t i, j;
  size_t enc_sz = 4 * ((sz + 2) / 3);

  tsunlikely_if( (*ret = (char *) malloc(enc_sz+1)) == NULL )
    return "OOM";
  
  (*ret)[enc_sz] = '\0';
  
  for (i = 0, j = 0; i < sz;) {
    char octet_a = i < sz ? data[i++] : 0;
    char octet_b = i < sz ? data[i++] : 0;
    char octet_c = i < sz ? data[i++] : 0;

    uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

    (*ret)[j++] = base64_enc_table[(triple >> 3 * 6) & 0x3F];
    (*ret)[j++] = base64_enc_table[(triple >> 2 * 6) & 0x3F];
    (*ret)[j++] = base64_enc_table[(triple >> 1 * 6) & 0x3F];
    (*ret)[j++] = base64_enc_table[(triple >> 0 * 6) & 0x3F];
  }
  
  for (int i = 0; i < mod_table[sz % 3]; i++)
    (*ret)[enc_sz - 1 - i] = '=';
  
  return NULL;
}

const char* ts_base64_dec(char **ret, size_t* retsz, char *data, size_t datsz) {
  static char base64_dec_table[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,    
  };
  size_t i, j;
  tsunlikely_if(datsz % 4 != 0)
    return "ts_base64_dec invalid data size";
  
  *retsz = datsz / 4 * 3;
  if(data[datsz-1] == '=') --(*retsz);
  if(data[datsz-2] == '=') --(*retsz);
  
  tsunlikely_if( (*ret = (char *) malloc(*retsz+1)) == NULL )
    return "OOM";
  
  (*ret)[*retsz] = '\0';
  
  for (i = 0, j = 0; i < datsz;) {
    char sextet_a = data[i] == '=' ? 0 : base64_dec_table[(unsigned char)data[i]]; ++i;
    char sextet_b = data[i] == '=' ? 0 : base64_dec_table[(unsigned char)data[i]]; ++i;
    char sextet_c = data[i] == '=' ? 0 : base64_dec_table[(unsigned char)data[i]]; ++i;
    char sextet_d = data[i] == '=' ? 0 : base64_dec_table[(unsigned char)data[i]]; ++i;
    
    if((sextet_a | sextet_b | sextet_c | sextet_d) & 0x80) {
      free(*ret);
      *ret = NULL;
      return "ts_base64_dec invalid data";
    }
    
    uint32_t triple = (sextet_a << 3 * 6) | (sextet_b << 2 * 6) | 
                      (sextet_c << 1 * 6) | (sextet_d << 0 * 6) ;

    if (j < *retsz) (*ret)[j++] = (triple >> 2 * 8) & 0xFF;
    if (j < *retsz) (*ret)[j++] = (triple >> 1 * 8) & 0xFF;
    if (j < *retsz) (*ret)[j++] = (triple >> 0 * 8) & 0xFF;
  }
  
  return NULL;
}
