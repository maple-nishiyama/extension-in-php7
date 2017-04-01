/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2017 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_myext.h"

/* If you declare any globals in php_myext.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(myext)
*/

/* True global resources - no need for thread safety here */
static int le_myext;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("myext.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_myext_globals, myext_globals)
    STD_PHP_INI_ENTRY("myext.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_myext_globals, myext_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_myext_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_myext_compiled)
{
	char *arg = NULL;
	size_t arg_len, len;
	zend_string *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	strg = strpprintf(0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "myext", arg);

	RETURN_STR(strg);
}

zend_class_entry* myext_ce;

static zend_object_handlers myext_object_handlers;

static inline php_myext* php_myext_fetch_object(zend_object* obj)
{
    return (php_myext*)((char*)obj - XtOffsetOf(php_myext, std));
}

#define Z_MYEXT_OBJ_P(zv) php_myext_fetch_object(Z_OBJ_P(zv))

/*
 * カスタムオブジェクトの生成関数（コンストラクタ）
 */
static zend_object* php_myext_new(zend_class_entry* ce)
{
    // メモリ領域を確保
    php_myext* obj = (php_myext*)ecalloc(1, sizeof(php_myext) + zend_object_properties_size(ce));
    // 標準オブジェクト部分の初期化
    zend_object_std_init(&obj->std, ce);
    // プロパティ部分の初期化
    object_properties_init(&obj->std, ce);
    obj->std.handlers = &myext_object_handlers;
    return &obj->std;
}

// 破棄関数（デストラクタ）
static void php_myext_destroy_object(zend_object* object)
{
    php_myext* obj = php_myext_fetch_object(object);
    zend_objects_destroy_object(object);
}

static void php_myext_object_free_storage(zend_object* object)
{
    php_myext* obj = php_myext_fetch_object(object);
    if (obj->buff) {
        efree(obj->buff);
    }
    zend_object_std_dtor(&obj->std);
}

// PHP 側のコンストラクタ (__construct() メソッド)
// PHP の配列を受け取って、それと同じ値が並ぶCの配列を確保する
PHP_METHOD(Myext, __construct)
{
    zval* a;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(a)
    ZEND_PARSE_PARAMETERS_END();

    HashTable* h = HASH_OF(a);

    php_myext* object = Z_MYEXT_OBJ_P(getThis());
    // C側のメモリーをここで確保
    zend_long s =  zend_hash_num_elements(h);
    object->size =s;
    object->buff = ecalloc(s, sizeof(double));

    // Cの配列に値をコピーしていく
    for (zend_long i = 0; i < s; ++i) {
        zval* elm = zend_hash_index_find(h, i);
        convert_to_double_ex(elm);
        object->buff[i] = Z_DVAL_P(elm);
    }
}

// C側の配列をPHPの配列として読み出すメソッド
PHP_METHOD(Myext, readBuffer)
{
    php_myext* object = Z_MYEXT_OBJ_P(getThis());

    zend_long s = object->size;
    zval out;
    array_init_size(&out, s);
    for (zend_long i = 0; i < s; ++i) {
        add_index_double(&out, i, object->buff[i]);
    }
    RETURN_ZVAL(&out, 1, 1);
}

// C側で Myext クラスを new して PHP 側に return するスタティックメソッド
PHP_METHOD(Myext, zeros)
{
    zend_long s;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(s)
    ZEND_PARSE_PARAMETERS_END();

    // zval* return_value という返り値用の変数がマクロ内で定義されているので
    // それをほしい形に加工する
    object_init_ex(return_value, myext_ce);
    Z_SET_REFCOUNT_P(return_value, 1);
    php_myext* object = Z_MYEXT_OBJ_P(return_value);
    object->size = s;
    object->buff = ecalloc(s, sizeof(double));
}

PHP_FUNCTION(my_sum)
{
    /* 引数の格納先 */
    zend_long a, b;

    /* 引数をパースして a, b に代入 */
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(a)
        Z_PARAM_LONG(b)
    ZEND_PARSE_PARAMETERS_END();

    /* 和を計算 */
    zend_long c = a + b;

    /* 結果を return する */
    RETURN_LONG(c);
}

PHP_METHOD(Myext, my_sum_method)
{
    zend_long a, b;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(a)
        Z_PARAM_LONG(b)
    ZEND_PARSE_PARAMETERS_END();

    zend_long c = a + b;

    RETURN_LONG(c);
}

/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_myext_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_myext_init_globals(zend_myext_globals *myext_globals)
{
	myext_globals->global_value = 0;
	myext_globals->global_string = NULL;
}
*/
/* }}} */

/*
 * メソッドの登録
 */
const zend_function_entry myext_methods[] = {
    PHP_ME(Myext, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Myext, readBuffer, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Myext, my_sum_method, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Myext, zeros, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_FE_END	/* Must be the last line in myext_functions[] */
};
/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(myext)
{
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/
    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "Myext", myext_methods);
    myext_ce = zend_register_internal_class(&ce);

    zend_declare_property_string(myext_ce, "hello", sizeof("hello") - 1, "hello, myext!", ZEND_ACC_PUBLIC);

    myext_ce->create_object = php_myext_new;

    memcpy(&myext_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    myext_object_handlers.offset = XtOffsetOf(php_myext, std);
    myext_object_handlers.clone_obj = NULL;
    myext_object_handlers.dtor_obj = php_myext_destroy_object;
    myext_object_handlers.free_obj = php_myext_object_free_storage;


	return SUCCESS;
}

/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(myext)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(myext)
{
#if defined(COMPILE_DL_MYEXT) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(myext)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(myext)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "myext support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ myext_functions[]
 *
 * Every user visible function must have an entry in myext_functions[].
 */
const zend_function_entry myext_functions[] = {
	PHP_FE(confirm_myext_compiled,	NULL)		/* For testing, remove later. */
    PHP_FE(my_sum, NULL)
    PHP_ME(Myext, my_sum_method, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END	/* Must be the last line in myext_functions[] */
};

/* }}} */

/* {{{ myext_module_entry
 */
zend_module_entry myext_module_entry = {
	STANDARD_MODULE_HEADER,
	"myext",
	myext_functions,
	PHP_MINIT(myext),
	PHP_MSHUTDOWN(myext),
	PHP_RINIT(myext),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(myext),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(myext),
	PHP_MYEXT_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_MYEXT
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(myext)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
