/* tests/test_server.c */
#include <CUnit/Basic.h>
#include "http_parser.h"  /* sua API a testar */


void test_char_to_int(void) {
    /* supondo que você tenha uma função: int char_to_int(char c); */
    //CU_ASSERT_EQUAL(char_to_int('0'), 0);
    //CU_ASSERT_EQUAL(char_to_int('1'), 1);
    //CU_ASSERT_NOT_EQUAL(char_to_int('9'), -1);
}

void test_buff_decode(void) {
    char s1[] = "/path/with%20spaces/file.txt";
    int len = strlen(s1);
    buffer_decode(s1, &len);
    char* decoded = mkstring(s1, len);    
    CU_ASSERT_EQUAL(len, strlen("/path/with spaces/file.txt"));
    CU_ASSERT_STRING_EQUAL(decoded, "/path/with spaces/file.txt");    
}

/* Adicione outros testes que cobrem as funções do server_core */

int main(void) {
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    CU_pSuite suite = CU_add_suite("server_core_suite", NULL, NULL);
    if (NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(suite, "test_char_to_int", test_char_to_int)) ||
        (NULL == CU_add_test(suite, "test_buff_decode", test_buff_decode))) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    unsigned int fails = CU_get_number_of_failures();
    CU_cleanup_registry();

    return (fails == 0) ? 0 : 1;
}
