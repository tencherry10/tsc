#ifndef TS_TEST_H__
#define TS_TEST_H__

#ifdef USE_TS_TEST

#define TEST_SUITE(name)                void name(void)

#define TEST_FUNC(name)                 static void name(void)
#define TEST_REG(name)                  ts_test_add_test(name, #name, __func__)
#define TEST_CASE(name)                 auto void name(void); TEST_REG(name); void name(void)

#define TEST_ASSERT(expr)               ts_test_assert_run((int)(expr), #expr, __func__, __FILE__, __LINE__)
#define TEST_ASSERT_STR_EQ(fst, snd)    ts_test_assert_run(\
  strcmp(fst, snd) == 0,                \
  "strcmp( " #fst ", " #snd " ) == 0",  \
  __func__, __FILE__, __LINE__)

#define TEST_ADD_SUITE(name)            ts_test_add_suite(name)
#define TEST_ADD_TEST(func,name,suite)  ts_test_add_test(func,name,suite)
#define TEST_RUN_ALL()                  ts_test_run_all()

TSC_EXTERN void   ts_test_assert_run(int result, const char* expr, const char* func, const char* file, int line);
TSC_EXTERN void   ts_test_add_test(void (*func)(void), const char* name, const char* suite);
TSC_EXTERN void   ts_test_add_suite(void (*func)(void));
TSC_EXTERN int    ts_test_run_all(void);

#endif

#endif
