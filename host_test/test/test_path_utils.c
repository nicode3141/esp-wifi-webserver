#include "unity.h"

void test_store_path_returns_valid_input(void){
    TEST_ASSERT_EQUAL(PATH_OK, store_path("index.html"));
    TEST_ASSERT_EQUAL(PATH_OK, store_path("One morning, when Gregor Samsa woke from troubled d"));
}

void test_store_path_returns_too_long(void){
    // Internal buffer only has 64 byte 64 - 11 - 1- 1 = 51
    TEST_ASSERT_EQUAL(PATH_ERR_TOO_LONG, store_path("One morning, when Gregor Samsa woke from troubled dr"));
}

void test_store_path_returns_null(void){
    char *str = NULL;
    TEST_ASSERT_EQUAL(PATH_ERR_NULL, store_path(str));
}