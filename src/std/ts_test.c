#include "libts.h"

#define USE_TS_TEST
#ifdef USE_TS_TEST

#define TSCT_MAX_NAME  512
#define TSCT_MAX_ERROR 2048
#define TSCT_MAX_TESTS 2048

enum {
  BLACK         = 0,
  BLUE          = 1,
  GREEN         = 2,
  AQUA          = 3,
  RED           = 4,
  PURPLE        = 5,
  YELLOW        = 6,
  WHITE         = 7,
  GRAY          = 8,
  LIGHT_BLUE    = 9,
  LIGHT_GREEN   = 10,
  LIGHT_AQUA    = 11,
  LIGHT_RED     = 12,
  LIGHT_PURPLE  = 13,
  LIGHT_YELLOW  = 14,
  LIGHT_WHITE   = 15,
  DEFAULT       = 16,
};

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

typedef struct {
  void (*func)(void);
  char name[TSCT_MAX_NAME];
  char suite[TSCT_MAX_NAME];
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
  char assert_err_buff[TSCT_MAX_ERROR];
  char assert_err[TSCT_MAX_ERROR];
} ts_test_info_t;

static ts_test_t       tests[TSCT_MAX_TESTS]  = {0};
static ts_test_info_t  test_info         = {0};

static void ts_test_color(int color) {  
  printf("%s", ts_test_colors[color]);
}

void ts_test_assert_run(int result, const char* expr, const char* func, const char* file, int line) {
  (void) func;
  test_info.num_asserts++;
  test_info.test_passing = test_info.test_passing && result;
  
  if (result) {
    test_info.num_assert_passes++;
  } else {
    sprintf(test_info.assert_err_buff, 
      "        %i. Assert [ %s ] (%s:%i)\n", 
      test_info.assert_err_num+1, expr, file, line );
    strcat(test_info.assert_err, test_info.assert_err_buff);
    test_info.assert_err_num++;
    test_info.num_assert_fails++;
  }
}

static void ts_test_signal(int sig) {
  test_info.test_passing = 0;
  switch( sig ) {
    case SIGFPE:  sprintf(test_info.assert_err_buff,
      "        %i. Division by Zero\n", test_info.assert_err_num+1);
    break;
    case SIGILL:  sprintf(test_info.assert_err_buff,
      "        %i. Illegal Instruction\n", test_info.assert_err_num+1);
    break;
    case SIGSEGV: sprintf(test_info.assert_err_buff,
      "        %i. Segmentation Fault\n", test_info.assert_err_num+1);
    break;
  }
  
  test_info.assert_err_num++;
  strcat(test_info.assert_err, test_info.assert_err_buff);
  
  ts_test_color(RED); 
  printf("Failed! \n\n%s\n", test_info.assert_err);
  ts_test_color(DEFAULT);
  
  puts("    | Stopping Execution.");
  fflush(stdout);
  exit(0);
}

static void ts_test_title_case(char* output, const char* input) {
  int space = 1;
  strcpy(output, input);
  for(unsigned int i = 0; i < strlen(output); i++) {
    if (output[i] == '_' || output[i] == ' ') {
      space = 1;
      output[i] = ' ';
      continue;
    } 
    
    if (space && output[i] >= 'a' && output[i] <= 'z') {
      space = 0;
      output[i] = output[i] - 32;
      continue;
    }
    
    space = 0;   
  }
}

void ts_test_add_test(void (*func)(void), const char* name, const char* suite) {
  if (test_info.num_tests == TSCT_MAX_TESTS) {
    printf("ERROR: Exceeded maximum test count of %i!\n", 
      TSCT_MAX_TESTS); abort();
  }
  
  if (strlen(name) >= TSCT_MAX_NAME) {
    printf("ERROR: Test name '%s' too long (Maximum is %i characters)\n", 
      name, TSCT_MAX_NAME); abort();
  }
  
  if (strlen(suite) >= TSCT_MAX_NAME) {
    printf("ERROR: Test suite '%s' too long (Maximum is %i characters)\n", 
      suite, TSCT_MAX_NAME); abort();
  }

  ts_test_t test;  
  test.func = func;
  ts_test_title_case(test.name, name);
  ts_test_title_case(test.suite, suite);
  
  tests[test_info.num_tests++] = test;
}

void ts_test_add_suite(void (*func)(void)) {
  test_info.num_suites++;
  func();
}

int ts_test_run_all(void) {
  static char current_suite[TSCT_MAX_NAME];
  
  signal(SIGFPE,  ts_test_signal);
  signal(SIGILL,  ts_test_signal);
  signal(SIGSEGV, ts_test_signal);
  
  clock_t start = clock();
  strcpy(current_suite, "");
  
  for(int i = 0; i < test_info.num_tests; i++) {
    ts_test_t test = tests[i];
    
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
      strcpy(current_suite, test.suite);
      printf("\n\n  ===== %s =====\n\n", current_suite);
    }
    
    /* Run Test */
    
    test_info.test_passing = 1;
    strcpy(test_info.assert_err, "");
    strcpy(test_info.assert_err_buff, "");
    test_info.assert_err_num = 0;
    printf("    | %s ... ", test.name);
    fflush(stdout);
    
    test.func();
    
    test_info.suite_passing = test_info.suite_passing && test_info.test_passing;
    
    if (test_info.test_passing) {
      test_info.num_tests_passes++;
      ts_test_color(GREEN);
      puts("Passed!");
      ts_test_color(DEFAULT);
    } else {
      test_info.num_tests_fails++;
      ts_test_color(RED); 
      printf("Failed! \n\n%s\n", test_info.assert_err);
      ts_test_color(DEFAULT);
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
  ts_test_color(YELLOW);  printf(" Total %4d ",  test_info.num_suites);        
  ts_test_color(DEFAULT); putchar('|');
  ts_test_color(GREEN);   printf(" Passed %4d ", test_info.num_suites_passes); 
  ts_test_color(DEFAULT); putchar('|');
  ts_test_color(RED);     printf(" Failed %4d ", test_info.num_suites_fails);  
  ts_test_color(DEFAULT); puts("|");
  
  printf("  | Tests   ||");
  ts_test_color(YELLOW);  printf(" Total %4d ",  test_info.num_tests);         
  ts_test_color(DEFAULT); putchar('|');
  ts_test_color(GREEN);   printf(" Passed %4d ", test_info.num_tests_passes);  
  ts_test_color(DEFAULT); putchar('|');
  ts_test_color(RED);     printf(" Failed %4d ", test_info.num_tests_fails);   
  ts_test_color(DEFAULT); puts("|");
  
  printf("  | Asserts ||");
  ts_test_color(YELLOW);  printf(" Total %4d ",  test_info.num_asserts);       
  ts_test_color(DEFAULT); putchar('|');
  ts_test_color(GREEN);   printf(" Passed %4d ", test_info.num_assert_passes); 
  ts_test_color(DEFAULT); putchar('|');
  ts_test_color(RED);     printf(" Failed %4d ", test_info.num_assert_fails);  
  ts_test_color(DEFAULT); puts("|");
  
  puts("  +---------++------------+-------------+-------------+");
  puts("");
  
  double total = (double)(end - start) / CLOCKS_PER_SEC;
  
  printf("      Total Running Time: %0.3fs\n\n", total);
  
  if (test_info.num_suites_fails > 0) { return 1; } else { return 0; }
}

#endif
