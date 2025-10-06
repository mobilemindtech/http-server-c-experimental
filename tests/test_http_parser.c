/* tests/test_server.c */
#include <CUnit/Basic.h>
#include "http_parser.h"  /* sua API a testar */

/*  TESTE DECODE START */
void test_buff_decode(void) {
    char s1[] = "/path/with%20spaces/file.txt";
    int len = strlen(s1);
    buffer_decode(s1, &len);
    char* decoded = mkstring(s1, len);    
    CU_ASSERT_EQUAL(len, strlen("/path/with spaces/file.txt"));
    CU_ASSERT_STRING_EQUAL(decoded, "/path/with spaces/file.txt");    
}

void test_buff_decode_query_simple(void) {
    char s1[] = "/search?q=hello%20world";
    int len = strlen(s1);
    buffer_decode(s1, &len);
    char* decoded = mkstring(s1, len);
    CU_ASSERT_EQUAL(len, strlen("/search?q=hello world"));
    CU_ASSERT_STRING_EQUAL(decoded, "/search?q=hello world");
}

void test_buff_decode_query_with_special_chars(void) {
    char s1[] = "/api/data?name=John%20Doe&city=New%20York";
    int len = strlen(s1);
    buffer_decode(s1, &len);
    char* decoded = mkstring(s1, len);
    CU_ASSERT_EQUAL(len, strlen("/api/data?name=John Doe&city=New York"));
    CU_ASSERT_STRING_EQUAL(decoded, "/api/data?name=John Doe&city=New York");
}

void test_buff_decode_percent_encoded_reserved(void) {
    char s1[] = "/test/%3Fparam%3Dvalue"; // %3F = '?', %3D = '='
    int len = strlen(s1);
    buffer_decode(s1, &len);
    char* decoded = mkstring(s1, len);
    CU_ASSERT_EQUAL(len, strlen("/test/?param=value"));
    CU_ASSERT_STRING_EQUAL(decoded, "/test/?param=value");
}

void test_buff_decode_plus_sign(void) {
    char s1[] = "/search?query=C%2B%2B+language"; // %2B = '+'
    int len = strlen(s1);
    buffer_decode(s1, &len);
    char* decoded = mkstring(s1, len);
    CU_ASSERT_EQUAL(len, strlen("/search?query=C++ language"));
    CU_ASSERT_STRING_EQUAL(decoded, "/search?query=C++ language");
}

void test_buff_decode_invalid_escape(void) {
    char s1[] = "/broken%2path"; // "%2p" inválido, deve manter literal
    int len = strlen(s1);
    buffer_decode(s1, &len);
    char* decoded = mkstring(s1, len);
    CU_ASSERT_EQUAL(len, strlen("/broken%2path"));
    CU_ASSERT_STRING_EQUAL(decoded, "/broken%2path");
}

/*  TESTE DECODE END */

/*  TESTE URL PARSE START */

void test_parse_url_query(void) {
    char s1[] = "/site?x=1&y=2";

    request_t* req = request_new();
    req->path = s1;
    req->path_len = strlen(s1);
    parse_url(req);
    request_debug(req);

    CU_ASSERT_EQUAL(5, req->path_len);
    char* path= mkstring(req->path, req->path_len);
    CU_ASSERT_STRING_EQUAL(path, "/site");

    CU_ASSERT_EQUAL(2, req->queries_len);

    char* xkey = mkstring(req->queries[0].key, req->queries[0].lkey);
    char* xval = mkstring(req->queries[0].val, req->queries[0].lval);
    char* ykey = mkstring(req->queries[1].key, req->queries[1].lkey);
    char* yval = mkstring(req->queries[1].val, req->queries[1].lval);

    CU_ASSERT_STRING_EQUAL(xkey, "x");
    CU_ASSERT_STRING_EQUAL(xval, "1");
    CU_ASSERT_STRING_EQUAL(ykey, "y");
    CU_ASSERT_STRING_EQUAL(yval, "2");
}

void test_parse_url_query_empty_value(void) {
    char s1[] = "/search?q=";  // valor vazio

    request_t* req = request_new();
    req->path = s1;
    req->path_len = strlen(s1);
    parse_url(req);

    CU_ASSERT_EQUAL(7, req->path_len);
    char* path= mkstring(req->path, req->path_len);
    CU_ASSERT_STRING_EQUAL(path, "/search");


    CU_ASSERT_EQUAL(1, req->queries_len);

    char* qkey = mkstring(req->queries[0].key, req->queries[0].lkey);
    char* qval = mkstring(req->queries[0].val, req->queries[0].lval);

    CU_ASSERT_STRING_EQUAL(qkey, "q");
    CU_ASSERT_STRING_EQUAL(qval, "");
}

void test_parse_url_query_empty_key(void) {
    char s1[] = "/test?=123";  // chave vazia

    request_t* req = request_new();
    req->path = s1;
    req->path_len = strlen(s1);
    parse_url(req);
    request_debug(req);

    CU_ASSERT_EQUAL(5, req->path_len);
    char* path= mkstring(req->path, req->path_len);
    CU_ASSERT_STRING_EQUAL(path, "/test");


    CU_ASSERT_EQUAL(1, req->queries_len);

    char* key = mkstring(req->queries[0].key, req->queries[0].lkey);
    char* val = mkstring(req->queries[0].val, req->queries[0].lval);

    CU_ASSERT_STRING_EQUAL(key, "");
    CU_ASSERT_STRING_EQUAL(val, "123");
}

void test_parse_url_query_no_value(void) {
    char s1[] = "/config?flag";  // parâmetro sem '='

    request_t* req = request_new();
    req->path = s1;
    req->path_len = strlen(s1);
    parse_url(req);

    CU_ASSERT_EQUAL(7, req->path_len);
    char* path= mkstring(req->path, req->path_len);
    CU_ASSERT_STRING_EQUAL(path, "/config");


    CU_ASSERT_EQUAL(1, req->queries_len);

    char* key = mkstring(req->queries[0].key, req->queries[0].lkey);
    char* val = mkstring(req->queries[0].val, req->queries[0].lval);

    CU_ASSERT_STRING_EQUAL(key, "flag");
    CU_ASSERT_STRING_EQUAL(val, "");
}

void test_parse_url_query_multiple_empty(void) {
    char s1[] = "/multi?x=&y=&z=";  // vários valores vazios

    request_t* req = request_new();
    req->path = s1;
    req->path_len = strlen(s1);
    parse_url(req);

    CU_ASSERT_EQUAL(6, req->path_len);
    char* path= mkstring(req->path, req->path_len);
    CU_ASSERT_STRING_EQUAL(path, "/multi");

    CU_ASSERT_EQUAL(3, req->queries_len);

    char* xval = mkstring(req->queries[0].val, req->queries[0].lval);
    char* yval = mkstring(req->queries[1].val, req->queries[1].lval);
    char* zval = mkstring(req->queries[2].val, req->queries[2].lval);

    CU_ASSERT_STRING_EQUAL(xval, "");
    CU_ASSERT_STRING_EQUAL(yval, "");
    CU_ASSERT_STRING_EQUAL(zval, "");
}

void test_parse_url_query_escaped(void) {
    char s1[] = "/path?city=New%20York&lang=C%2B%2B";  // com escapes

    request_t* req = request_new();
    req->path = s1;
    req->path_len = strlen(s1);
    parse_url(req);

    CU_ASSERT_EQUAL(5, req->path_len);
    char* path= mkstring(req->path, req->path_len);
    CU_ASSERT_STRING_EQUAL(path, "/path");

    CU_ASSERT_EQUAL(2, req->queries_len);

    char* ckey = mkstring(req->queries[0].key, req->queries[0].lkey);
    char* cval = mkstring(req->queries[0].val, req->queries[0].lval);
    char* lkey = mkstring(req->queries[1].key, req->queries[1].lkey);
    char* lval = mkstring(req->queries[1].val, req->queries[1].lval);

    CU_ASSERT_STRING_EQUAL(ckey, "city");
    CU_ASSERT_STRING_EQUAL(cval, "New%20York");
    CU_ASSERT_STRING_EQUAL(lkey, "lang");
    CU_ASSERT_STRING_EQUAL(lval, "C%2B%2B");
}

/*  TESTE URL PARSE END */

/*  TESTE REQUEST PARSE START */

// ------------------------------------------------------------------
// Teste 1: GET simples
// ------------------------------------------------------------------
void test_simple_get(void) {
    request_t* req = request_new();
    char buf[] = "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n";
    int len = strlen(buf);
    int ret = 0;

    parse_request(buf, &len, req, &ret);
    CU_ASSERT_EQUAL(ret, 0);

    char *method = mkstring(req->method, req->method_len);
    char *path = mkstring(req->path, req->path_len);
    char *hkey = mkstring(req->headers[0].key, req->headers[0].lkey);
    char *hval = mkstring(req->headers[0].val, req->headers[0].lval);

    CU_ASSERT_STRING_EQUAL(method, "GET");
    CU_ASSERT_STRING_EQUAL(path, "/index.html");
    CU_ASSERT_EQUAL(req->minor_version, 1);
    CU_ASSERT_STRING_EQUAL(hkey, "Host");
    CU_ASSERT_STRING_EQUAL(hval, "example.com");

    free(method);
    free(path);
    free(hkey);
    free(hval);
}

// ------------------------------------------------------------------/ Teste 2: GET com query
// ------------------------------------------------------------------
void test_get_with_query(void) {
    request_t* req = request_new();
    char buf[] = "GET /search?q=test&page=2 HTTP/1.1\r\nHost: site.com\r\n\r\n";
    int len = strlen(buf);
    int ret = 0;

    parse_request(buf, &len, req, &ret);
    CU_ASSERT_EQUAL(ret, 0);

    char *path = mkstring(req->path, req->path_len);
    CU_ASSERT_STRING_EQUAL(path, "/search?q=test&page=2");

    CU_ASSERT_EQUAL(req->headers_len, 1);

    char *h1k = mkstring(req->headers[0].key, req->headers[0].lkey);
    char *h1v = mkstring(req->headers[0].val, req->headers[0].lval);

    CU_ASSERT_STRING_EQUAL(h1k, "Host");
    CU_ASSERT_STRING_EQUAL(h1v, "site.com");

    free(path); free(h1k); free(h1v); free(req);
}

// ------------------------------------------------------------------
// Teste 3: POST com corpo
// ------------------------------------------------------------------
void test_post_with_body(void) {
    request_t* req = request_new();
    int ret = 0;
    char buf[] =
        "POST /submit HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Length: 12\r\n"
        "\r\n"
        "name=Ricardo";
    int len = strlen(buf);

    parse_request(buf, &len, req, &ret);
    CU_ASSERT_EQUAL(ret, 0);

    char *method = mkstring(req->method, req->method_len);
    char *path = mkstring(req->path, req->path_len);
    char *body = mkstring(req->body, req->body_len);

    CU_ASSERT_STRING_EQUAL(method, "POST");
    CU_ASSERT_STRING_EQUAL(path, "/submit");
    CU_ASSERT_STRING_EQUAL(body, "name=Ricardo");

    free(method);
    free(path);
    free(body);
}

// ------------------------------------------------------------------
// Teste 4: Request inválido (sem HTTP version)
// ------------------------------------------------------------------
void test_invalid_missing_version(void) {
    request_t* req = request_new();
    int ret = 0;
    char buf[] = "GET /index.html\r\nHost: example.com\r\n\r\n";
    int len = strlen(buf);

    parse_request(buf, &len, req, &ret);
    CU_ASSERT_NOT_EQUAL(ret, 0);
}

// ------------------------------------------------------------------
// Teste 5: Header malformado
// ------------------------------------------------------------------
void test_invalid_header_format(void) {
    request_t* req = request_new();
    int ret = 0;
    char buf[] = "GET / HTTP/1.1\r\nHost example.com\r\n\r\n"; // falta ':'
    int len = strlen(buf);

    parse_request(buf, &len, req, &ret);
    CU_ASSERT_NOT_EQUAL(ret, 0);
}

// ------------------------------------------------------------------
// Teste 6: Path com %20
// ------------------------------------------------------------------
void test_path_with_escape(void) {
    request_t* req = request_new();
    int ret = 0;
    char buf[] = "GET /my%20docs/test.txt HTTP/1.1\r\nHost: a.com\r\n\r\n";
    int len = strlen(buf);

    parse_request(buf, &len, req, &ret);
    CU_ASSERT_EQUAL(ret, 0);

    char *path = mkstring(req->path, req->path_len);
    CU_ASSERT_STRING_EQUAL(path, "/my%20docs/test.txt");
    free(path);
}

// ------------------------------------------------------------------
// Teste 7: Múltiplos headers
// ------------------------------------------------------------------
void test_multiple_headers(void) {
    request_t* req = request_new();
    int ret = 0;
    char buf[] =
        "GET /api/data HTTP/1.1\r\n"
        "Host: api.local\r\n"
        "User-Agent: curl/7.68.0\r\n"
        "Accept: */*\r\n"
        "\r\n";
    int len = strlen(buf);

    parse_request(buf, &len, req, &ret);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(req->headers_len, 3);

    char *key = mkstring(req->headers[1].key, req->headers[1].lkey);
    char *val = mkstring(req->headers[1].val, req->headers[1].lval);
    CU_ASSERT_STRING_EQUAL(key, "User-Agent");
    CU_ASSERT_STRING_EQUAL(val, "curl/7.68.0");

    free(key);
    free(val);
}


/*  TESTE REQUEST PARSE END */

/* Adicione outros testes que cobrem as funções do server_core */

int main(void) {
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    CU_pSuite suite = CU_add_suite("server_core_suite", NULL, NULL);
    if (NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    int added =
        (CU_add_test(suite, "test_buff_decode", test_buff_decode) != NULL)
        && (CU_add_test(suite, "test_buff_decode_query_simple", test_buff_decode_query_simple) != NULL)
        && (CU_add_test(suite, "test_buff_decode_query_with_special_chars", test_buff_decode_query_with_special_chars) != NULL)
        && (CU_add_test(suite, "test_buff_decode_percent_encoded_reserved", test_buff_decode_percent_encoded_reserved) != NULL)
        && (CU_add_test(suite, "test_buff_decode_percent_encoded_reserved", test_buff_decode_plus_sign) != NULL)
        && (CU_add_test(suite, "test_buff_decode_invalid_escape", test_buff_decode_invalid_escape) != NULL)
        && (CU_add_test(suite, "test_parse_url_query", test_parse_url_query) != NULL)
        && (CU_add_test(suite, "test_parse_url_query_empty_key", test_parse_url_query_empty_key) != NULL)
        && (CU_add_test(suite, "test_parse_url_query_empty_value", test_parse_url_query_empty_value) != NULL)
        && (CU_add_test(suite, "test_parse_url_query_escaped", test_parse_url_query_escaped) != NULL)
        && (CU_add_test(suite, "test_parse_url_query_multiple_empty", test_parse_url_query_multiple_empty) != NULL)
        && (CU_add_test(suite, "test_parse_url_query_no_value", test_parse_url_query_no_value) != NULL)

        && (CU_add_test(suite, "GET simples", test_simple_get) != NULL)
        && (CU_add_test(suite, "GET com query", test_get_with_query) != NULL)
        && (CU_add_test(suite, "POST com body", test_post_with_body) != NULL)
        && (CU_add_test(suite, "Falta HTTP version", test_invalid_missing_version) != NULL)
        && (CU_add_test(suite, "Header malformado", test_invalid_header_format) != NULL)
        && (CU_add_test(suite, "Path com escape", test_path_with_escape) != NULL)
        && (CU_add_test(suite, "Múltiplos headers", test_multiple_headers) != NULL)
        ;


    if (!added) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    unsigned int fails = CU_get_number_of_failures();
    CU_cleanup_registry();

    return (fails == 0) ? 0 : 1;
}
