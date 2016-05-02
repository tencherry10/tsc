#include "libts.h"

#ifdef USE_TS_TEST

enum {
  TS_BLACK         = 0,
  TS_BLUE          = 1,
  TS_GREEN         = 2,
  TS_AQUA          = 3,
  TS_RED           = 4,
  TS_PURPLE        = 5,
  TS_YELLOW        = 6,
  TS_WHITE         = 7,
  TS_GRAY          = 8,
  TS_LIGHT_BLUE    = 9,
  TS_LIGHT_GREEN   = 10,
  TS_LIGHT_AQUA    = 11,
  TS_LIGHT_RED     = 12,
  TS_LIGHT_PURPLE  = 13,
  TS_LIGHT_YELLOW  = 14,
  TS_LIGHT_WHITE   = 15,
  TS_DEFAULT       = 16,
};

typedef struct {
  void (*func)(void);
  sds name;
  sds suite;
} ts_test_t;

typedef struct {
  int test_passing;
  int suite_passing;
  int num_asserts;
  int num_assert_passes;
  int num_assert_fails;
  int num_suites;
  int num_suites_passes;
  int num_suites_fails;  
  int num_tests;
  int num_tests_passes;
  int num_tests_fails;
  int assert_err_num;
  sds assert_err;
} ts_test_info_t;

ts_make_vec(tests_vec, ts_test_t)
static tests_vec_t    tests     = {0};
static ts_test_info_t test_info = {0};

static void ts_test_color(int color) {
  static const char* ts_test_colors[] = {
    "\x1B[0m",
    "\x1B[34m",
    "\x1B[32m",
    "\x1B[36m",
    "\x1B[31m",
    "\x1B[35m",
    "\x1B[33m",
    "\x1B[37m",
    "",
    "\x1B[34m",
    "\x1B[32m",
    "\x1B[36m",
    "\x1B[31m",
    "\x1B[35m",
    "\x1B[33m",
    "\x1B[37m",
    "\x1B[39m",
  };
  
  printf("%s", ts_test_colors[color]);
}

void ts_test_assert_run(int result, const char* expr, const char* func, const char* file, int line) {
  (void) func;
  test_info.num_asserts++;
  test_info.test_passing = test_info.test_passing && result;
  
  if (result) {
    test_info.num_assert_passes++;
  } else {
    test_info.assert_err = sdscatprintf(test_info.assert_err, 
      "        %i. Assert [ %s ] (%s:%i)\n", 
      test_info.assert_err_num+1, expr, file, line);
    test_info.assert_err_num++;
    test_info.num_assert_fails++;
  }
}

static void ts_test_signal(int sig) {
  test_info.test_passing = 0;
  switch( sig ) {
    case SIGFPE:  test_info.assert_err = sdscatprintf(test_info.assert_err,
      "        %i. Division by Zero\n", test_info.assert_err_num+1);
    break;
    case SIGILL:  test_info.assert_err = sdscatprintf(test_info.assert_err,
      "        %i. Illegal Instruction\n", test_info.assert_err_num+1);
    break;
    case SIGSEGV: test_info.assert_err = sdscatprintf(test_info.assert_err,
      "        %i. Segmentation Fault\n", test_info.assert_err_num+1);
    break;
  }
  test_info.assert_err_num++;
  ts_test_color(TS_RED); 
  printf("Failed! \n\n%s\n", test_info.assert_err);
  ts_test_color(TS_DEFAULT);
  
  puts("    | Stopping Execution.");
  fflush(stdout);
  exit(0);
}

static void ts_test_title_case(sds * output, const char* input) {
  int space = 1;
  *output = sdscpy(*output, input);
  
  for(unsigned int i = 0; i < sdslen(*output); i++) {
    if ((*output)[i] == '_' || (*output)[i] == ' ') {
      space = 1;
      (*output)[i] = ' ';
      continue;
    } 
    
    if (space && (*output)[i] >= 'a' && (*output)[i] <= 'z') {
      space = 0;
      (*output)[i] = (*output)[i] - 32;
      continue;
    }
    
    space = 0;   
  }
}

void ts_test_add_test(void (*func)(void), const char* name, const char* suite) {
  ts_test_t test;
  test.name   = sdsempty();
  test.suite  = sdsempty();
  test.func   = func;
  ts_test_title_case(&test.name, name);
  ts_test_title_case(&test.suite, suite);
  
  tests_vec_push(&tests, test);
  test_info.num_tests++;
  //~ tests[test_info.num_tests++] = test;
}

void ts_test_add_suite(void (*func)(void)) {
  test_info.num_suites++;
  func();
}

int ts_test_run_all(void) {
  test_info.assert_err = sdsempty();
  
  signal(SIGFPE,  ts_test_signal);
  signal(SIGILL,  ts_test_signal);
  signal(SIGSEGV, ts_test_signal);
  
  auto_sds current_suite = sdsempty();
  clock_t start = clock();
  
  for(int i = 0; i < test_info.num_tests; i++) {
    ts_test_t test = tests_vec_elem(&tests, i);
    
    /* Check for transition to a new suite */
    if (strcmp(test.suite, current_suite)) {

      /* Don't increment any counter for first entrance */
      if (strcmp(current_suite, "")) {
        if (test_info.suite_passing) {
          test_info.num_suites_passes++;
        } else {
          test_info.num_suites_fails++;
        }
      }
    
      test_info.suite_passing = 1;
      current_suite = sdscpy(current_suite, test.suite);
      printf("\n\n  ===== %s =====\n\n", current_suite);
    }
    
    /* Run Test */
    
    test_info.test_passing = 1;
    sdsclear(test_info.assert_err);
    test_info.assert_err_num = 0;
    printf("    | %s ... ", test.name);
    fflush(stdout);
    
    test.func();
    
    test_info.suite_passing = test_info.suite_passing && test_info.test_passing;
    
    if (test_info.test_passing) {
      test_info.num_tests_passes++;
      ts_test_color(TS_GREEN);
      puts("Passed!");
      ts_test_color(TS_DEFAULT);
    } else {
      test_info.num_tests_fails++;
      ts_test_color(TS_RED); 
      printf("Failed! \n\n%s\n", test_info.assert_err);
      ts_test_color(TS_DEFAULT);
    }
    
  }
  
  if (test_info.suite_passing) {
    test_info.num_suites_passes++;
  } else {
    test_info.num_suites_fails++;
  }
  
  clock_t end = clock();
  
  puts("");
  puts("  +---------------------------------------------------+");
  puts("  |                      Summary                      |");
  puts("  +---------++------------+-------------+-------------+");
  
  printf("  | Suites  ||");
  ts_test_color(TS_YELLOW);  printf(" Total %4d ",  test_info.num_suites);        
  ts_test_color(TS_DEFAULT); putchar('|');
  ts_test_color(TS_GREEN);   printf(" Passed %4d ", test_info.num_suites_passes); 
  ts_test_color(TS_DEFAULT); putchar('|');
  ts_test_color(TS_RED);     printf(" Failed %4d ", test_info.num_suites_fails);  
  ts_test_color(TS_DEFAULT); puts("|");
  
  printf("  | Tests   ||");
  ts_test_color(TS_YELLOW);  printf(" Total %4d ",  test_info.num_tests);         
  ts_test_color(TS_DEFAULT); putchar('|');
  ts_test_color(TS_GREEN);   printf(" Passed %4d ", test_info.num_tests_passes);  
  ts_test_color(TS_DEFAULT); putchar('|');
  ts_test_color(TS_RED);     printf(" Failed %4d ", test_info.num_tests_fails);   
  ts_test_color(TS_DEFAULT); puts("|");
  
  printf("  | Asserts ||");
  ts_test_color(TS_YELLOW);  printf(" Total %4d ",  test_info.num_asserts);       
  ts_test_color(TS_DEFAULT); putchar('|');
  ts_test_color(TS_GREEN);   printf(" Passed %4d ", test_info.num_assert_passes); 
  ts_test_color(TS_DEFAULT); putchar('|');
  ts_test_color(TS_RED);     printf(" Failed %4d ", test_info.num_assert_fails);  
  ts_test_color(TS_DEFAULT); puts("|");
  
  puts("  +---------++------------+-------------+-------------+");
  puts("");
  
  double total = (double)(end - start) / CLOCKS_PER_SEC;
  
  printf("      Total Running Time: %0.3fs\n\n", total);
  
  size_t i;
  ts_test_t t;
  ts_vec_foreach(tests, t, i) {
    sdsfree(t.name);
    sdsfree(t.suite);
  }
  tests_vec_destroy(&tests);
  sdsfree(test_info.assert_err);
  
  if (test_info.num_suites_fails > 0) { return 1; } else { return 0; }
}

#endif
