/**
 * @file
 * Unit tests of chkmalloc.
 *
 * @author John Young
 */

#include "chkmalloc.h"

#include <glib.h>
#include <errno.h>
#include <stdlib.h>

#define SIZE 1000
#define FAIL_SIZE ((size_t)-1)

/* Memory allocations etc. that should succeed */
static void test_succeed(void)
{
  assert_not_null((void *)1);

  assert_no_error(0, 0);

  void *ptr1 = chkmalloc(SIZE);
  assert_not_null(ptr1);
  free(ptr1);

  void *ptr2 = chkmalloc(SIZE);
  assert_not_null(ptr2);
  ptr2 = chkrealloc(ptr2, 2 * SIZE);
  assert_not_null(ptr2);
  free(ptr2);
}

#define TEST_SUBPROCESS_FAILS(cmd)                                             \
  do                                                                           \
  {                                                                            \
    if (g_test_subprocess())                                                   \
    {                                                                          \
      cmd;                                                                     \
      exit(0);                                                                 \
    }                                                                          \
    g_test_trap_subprocess(NULL, 0, 0);                                        \
    g_test_trap_assert_failed();                                               \
  } while (0);

/* Memory allocations etc. that should fail */
static void test_fail(void)
{
  TEST_SUBPROCESS_FAILS(assert_not_null(0));
  TEST_SUBPROCESS_FAILS(assert_no_error(-1, EINVAL));
  TEST_SUBPROCESS_FAILS(chkmalloc(0));
  TEST_SUBPROCESS_FAILS(chkmalloc(FAIL_SIZE));
  TEST_SUBPROCESS_FAILS(chkrealloc(chkmalloc(SIZE), 0));
  TEST_SUBPROCESS_FAILS(chkrealloc(chkmalloc(SIZE), FAIL_SIZE));
}

int main(int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/chkmalloc/succeed", test_succeed);
  g_test_add_func("/chkmalloc/fail", test_fail);

  return g_test_run();
}
