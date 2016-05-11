#define USE_TS_TEST
#include "tsc.h"

void base64_enc_test1(void) {
  auto_cstr enc;
  char *test1 = "Man is distinguished, not only by his reason, " \
      "but by this singular passion from other animals, which is a lust of the mind, "  \
      "that by a perseverance of delight in the continued and indefatigable generation of knowledge, " \
      "exceeds the short vehemence of any carnal pleasure.";
  char *res1  = "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz"  \
                "IHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg"  \
                "dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu"  \
                "dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo"  \
                "ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=";
  TEST_ASSERT(ts_base64_enc(&enc, test1, strlen(test1))==NULL);
  TEST_ASSERT(strcmp(enc,res1) == 0);
}

void base64_dec_test1(void) {
  size_t sz;
  auto_cstr dec;
  char *test1 = "Man is distinguished, not only by his reason, " \
      "but by this singular passion from other animals, which is a lust of the mind, "  \
      "that by a perseverance of delight in the continued and indefatigable generation of knowledge, " \
      "exceeds the short vehemence of any carnal pleasure.";
  char *res1  = "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz"  \
                "IHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg"  \
                "dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu"  \
                "dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo"  \
                "ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=";
  
  TEST_ASSERT(ts_base64_dec(&dec, &sz, res1, strlen(res1))==NULL);
  TEST_ASSERT(strcmp(dec,test1) == 0);
}

void base64_dec_invalid_char(void) {
  size_t sz;
  auto_cstr dec;
  char *res1  = "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz"  \
                "IHNpbmd1bGFyIHBhc3Npb24gZnJ.bSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg"  \
                "dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu"  \
                "dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo"  \
                "ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=";
    
  TEST_ASSERT(ts_base64_dec(&dec, &sz, res1, strlen(res1))!=NULL);
}


void suite_base64(void) {
  TEST_REG(base64_enc_test1);
  TEST_REG(base64_dec_test1);
  TEST_REG(base64_dec_invalid_char);
}

void matrix_alloc_2d_test(void) {
  int **mat2d;
  
  TEST_ASSERT(ts_alloc2d((void***) &mat2d,3,3,4) == NULL);

  for(int i = 0 ; i < 3 ; i++)
    for(int j = 0 ; j < 3 ; j++)
      mat2d[i][j] = i*100+j;

  for(int i = 0 ; i < 3 ; i++)
    for(int j = 0 ; j < 3 ; j++)
      mat2d[i][j] = mat2d[i][j] * mat2d[i][j];  // multiplied with itself

  int sum = 0;
  int sum_exp = 0;
  for(int i = 0 ; i < 3 ; i++)
    for(int j = 0 ; j < 3 ; j++)
      sum_exp += (i*100+j) * (i*100+j);
  for(int i = 0 ; i < 3 ; i++)
    for(int j = 0 ; j < 3 ; j++)  
      sum += mat2d[i][j];
  
  TEST_ASSERT(sum == sum_exp);
  
  free(mat2d);
}

void suite_matrix_alloc(void) {
  TEST_REG(matrix_alloc_2d_test);
}

ts_make_vec(int_vec, int)
ts_make_vec_extra(int_vec, int)

void vec_basic(void) {
  int_vec_t a;
  int_vec_init(&a);
  int_vec_push(&a, 0);
  int_vec_push(&a, 1);
  int_vec_push(&a, 2);
  int_vec_push(&a, 3);
  int_vec_push(&a, 4);
  
  TEST_ASSERT(0 == int_vec_first(&a));
  TEST_ASSERT(4 == int_vec_last(&a));
  TEST_ASSERT(5 == int_vec_size(&a));
  TEST_ASSERT(3 == int_vec_at(&a, 3));
  TEST_ASSERT(1 == int_vec_any(&a));
  TEST_ASSERT(0 == int_vec_all(&a));
  TEST_ASSERT(4 == int_vec_pop(&a));
  TEST_ASSERT(4 == int_vec_size(&a));
  TEST_ASSERT(8 == int_vec_max(&a));
  
  int_vec_push(&a, 10);
  int_vec_pushfront(&a, -10);
  
  int_vec_destroy(&a);
}

void vec_foreach(void) {
  size_t i;
  int idx, v, sum, *vp;
  int_vec_t a;
  int_vec_init(&a);
  int_vec_push(&a,  10);
  int_vec_push(&a,  20);
  int_vec_push(&a,  30);
  int_vec_push(&a,  40);
  int_vec_push(&a,  50);
  int_vec_push(&a,  60);
  int_vec_push(&a,  70);
  int_vec_push(&a,  80);
  int_vec_push(&a,  90);
  int_vec_push(&a, 100);
  
  sum = 0; 
  ts_vec_foreach(a, v, i)
    sum += v;
  
  TEST_ASSERT(550 == sum);
  
  ts_vec_foreach_ptr(a, vp, i)
    *vp = i * *vp;
  
  sum = 0; 
  ts_vec_foreach(a, v, i)
    sum += v;
  
  TEST_ASSERT(3300 == sum);
  
  idx = 0;
  sum = 0; 
  ts_vec_foreach_rev(a, v, i) {
    if( idx % 2 == 0) {
      sum += v;
    } else {
      sum -= v;
    }
    idx++;
  }
  TEST_ASSERT(500 == sum);
  
  int_vec_destroy(&a);
}


void suite_vec(void) {
  TEST_REG(vec_basic);
  TEST_REG(vec_foreach);
}

int main(int argc, const char ** argv) {
  (void)argc;
  (void)argv;
  TEST_ADD_SUITE(suite_base64);
  TEST_ADD_SUITE(suite_matrix_alloc);
  TEST_ADD_SUITE(suite_vec);
  
  //~ size_t    ndirs;
  //~ auto_cstr dirs_ptr  = NULL;
  //~ char      ***dirs   = (char ***) &dirs_ptr;  
  
  //~ ts_lsdir(dirs, &ndirs, ".");
  //~ for(size_t i = 0 ; i < ndirs ; i++) {
    //~ printf("%s\n", (*dirs)[i]);
  //~ }
  
  return TEST_RUN_ALL();
}



#define TSC_DEFINE
#include "tsc.h"
