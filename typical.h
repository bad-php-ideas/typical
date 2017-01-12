#ifndef incl_PHP_TYPICAL_H
#define incl_PHP_TYPICAL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

ZEND_BEGIN_MODULE_GLOBALS(typical)
	HashTable *types;
ZEND_END_MODULE_GLOBALS(typical)

typedef struct _php_typical_type_node php_typical_type_node;

#define PHP_TYPICAL_FLAG_ALLOW_NULL (1<<0)

#define PHP_TYPICAL_MATCH_TYPE 0
#define PHP_TYPICAL_CALLBACK   1
#define PHP_TYPICAL_JOIN_AND   2
#define PHP_TYPICAL_JOIN_OR    3
#define PHP_TYPICAL_JOIN_XOR   4

struct _php_typical_type_node {
	zend_uchar flags;
	zend_uchar join;
	union {
		struct {
			zend_uchar type_hint;
			zend_string *class_name;
		} type;
		zval callback;
		struct {
			php_typical_type_node *left;
			php_typical_type_node *right;
		} binary;
	} u;
};

typedef struct _php_typical_parser_extra {
	php_typical_type_node *ret;
	zend_bool syntax_error;
} php_typical_parser_extra;

void php_typical_destroy_type_node(php_typical_type_node *node);

#if defined(ZTS) && defined(COMPILE_DL_TYPICAL)
ZEND_TSRMLS_CACHE_EXTERN();
#endif

ZEND_EXTERN_MODULE_GLOBALS(typical)
#define TYPICALG(v) ZEND_MODULE_GLOBALS_ACCESSOR(typical, v)

#endif
