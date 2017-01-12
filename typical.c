/*
  +----------------------------------------------------------------------+
  | PHP Version 7							|
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group				|
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is	|
  | available through the world-wide-web at the following url:	   |
  | http://www.php.net/license/3_01.txt				  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to	  |
  | license@php.net so we can mail you a copy immediately.	       |
  +----------------------------------------------------------------------+
  | Author: Sara Golemon <pollita@php.net>			       |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "typical.h"

#include "ext/standard/php_string.h"

#if PHP_MAJOR_VERSION < 7
# error Typical requires PHP version 7 or later
#endif

ZEND_DECLARE_MODULE_GLOBALS(typical);

static php_typical_type_node *typical_lookup_custom_type(const zend_string *type_name);
static int typical_verify_type_name(zend_string *type_name, zval *val, zend_bool strict);

/* Dumb parser for now.  Just basic matchtypes with no whitespace allowances */
static php_typical_type_node* typical_parse_type(zend_string *type) {
	php_typical_type_node *node = ecalloc(1, sizeof(php_typical_type_node));
	const char *p = ZSTR_VAL(type);

	node->join = PHP_TYPICAL_MATCH_TYPE;
	if (p[0] == '?') {
		node->flags |= PHP_TYPICAL_FLAG_ALLOW_NULL;
		++p;
	}

	     if (!strcasecmp("bool", p))     { node->u.type.type_hint = _IS_BOOL; }
	else if (!strcasecmp("int", p))      { node->u.type.type_hint = IS_LONG; }
	else if (!strcasecmp("float", p))    { node->u.type.type_hint = IS_DOUBLE; }
	else if (!strcasecmp("string", p))   { node->u.type.type_hint = IS_STRING; }
	else if (!strcasecmp("array", p))    { node->u.type.type_hint = IS_ARRAY; }
	else if (!strcasecmp("callable", p)) { node->u.type.type_hint = IS_CALLABLE; }
	else if (!strcasecmp("iterable", p)) { node->u.type.type_hint = IS_ITERABLE; }
	else {
		node->u.type.type_hint = IS_OBJECT;
		node->u.type.class_name = zend_string_init(p, ZSTR_LEN(type) - (p - ZSTR_VAL(type)), 0);
		php_strtolower(ZSTR_VAL(node->u.type.class_name), ZSTR_LEN(node->u.type.class_name));
	}

	return node;
}

/* {{{ */
static void typical_destroy_type_node(php_typical_type_node *node) {
	if (!node) return;
	switch (node->join) {
		case PHP_TYPICAL_MATCH_TYPE:
			if (node->u.type.class_name) {
				zend_string_release(node->u.type.class_name);
			}
			break;
		case PHP_TYPICAL_CALLBACK:
			zval_dtor(&node->u.callback);
			break;
		case PHP_TYPICAL_JOIN_AND:
		case PHP_TYPICAL_JOIN_OR:
		case PHP_TYPICAL_JOIN_XOR:
			typical_destroy_type_node(node->u.binary.left);
			typical_destroy_type_node(node->u.binary.right);
			break;
	}
	efree(node);
}
static void typical_destroy_type_alias(zval *zv) {
	typical_destroy_type_node(Z_PTR_P(zv));
}
/* }}} */

/* {{{ proto void typical_set_type(string $alias, string $type) */
ZEND_BEGIN_ARG_INFO_EX(typical_set_type_arginfo, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_INFO(0, alias)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO();
static PHP_FUNCTION(typical_set_type) {
	zend_string *alias, *type;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_STR(alias)
		Z_PARAM_STR(type)
	ZEND_PARSE_PARAMETERS_END();

	if (!TYPICALG(types)) {
		ALLOC_HASHTABLE(TYPICALG(types));
		zend_hash_init(TYPICALG(types), 16, NULL, typical_destroy_type_alias, 0);
	}
	if (ZSTR_LEN(type) <= 0) {
		zend_hash_del(TYPICALG(types), type);
	} else {
		zend_string *lalias = php_string_tolower(alias);
		zval node;
		ZVAL_PTR(&node, typical_parse_type(type));
		zend_hash_update(TYPICALG(types), lalias, &node);
		zend_string_release(lalias);
	}
} /* }}} */

/* {{{ proto void typical_set_callback(string $alias, Callable $callback[, int $flags = 0]) */
ZEND_BEGIN_ARG_INFO_EX(typical_set_callback_arginfo, 0, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_INFO(0, alias)
	ZEND_ARG_INFO(0, callback)
	ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO();
static PHP_FUNCTION(typical_set_callback) {
	zend_string *alias, *lalias;
	zend_long flags = 0;
	php_typical_type_node *node;
	zval *callback, znode;

	ZEND_PARSE_PARAMETERS_START(2, 3)
		Z_PARAM_STR(alias)
		Z_PARAM_ZVAL_DEREF(callback)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(flags)
	ZEND_PARSE_PARAMETERS_END();

	if (!zend_is_callable(callback, IS_CALLABLE_CHECK_SILENT, NULL)) {
		php_error(E_ERROR, "Argument 2 passed to typical_set_callback must be a valid callback");
		return;
	}

	if (!TYPICALG(types)) {
		ALLOC_HASHTABLE(TYPICALG(types));
		zend_hash_init(TYPICALG(types), 16, NULL, typical_destroy_type_alias, 0);
	}

	node = ecalloc(1, sizeof(php_typical_type_node));
	node->flags = flags;
	node->join = PHP_TYPICAL_CALLBACK;
	ZVAL_ZVAL(&node->u.callback, callback, 1, 0);

	ZVAL_PTR(&znode, node);
	lalias = php_string_tolower(alias);
	zend_hash_update(TYPICALG(types), lalias, &znode);
	zend_string_release(lalias);
} /* }}} */

static void typical_get_type(zval *return_value, php_typical_type_node *node) {
	if (!node) { RETURN_NULL(); }

	array_init(return_value);
	add_assoc_long(return_value, "flags", node->flags);
	add_assoc_long(return_value, "join", node->join);
	if (node->join == PHP_TYPICAL_MATCH_TYPE) {
		add_assoc_long(return_value, "type_hint", node->u.type.type_hint);
		if (node->u.type.class_name) {
			add_assoc_str(return_value, "class_name", node->u.type.class_name);
		} else {
			add_assoc_null(return_value, "class_name");
		}
	} else if (node->join == PHP_TYPICAL_CALLBACK) {
		add_assoc_zval(return_value, "callback", &node->u.callback);
	} else {
		zval left, right;
		typical_get_type(&left, node->u.binary.left);
		typical_get_type(&right, node->u.binary.right);
		add_assoc_zval(return_value, "left", &left);
		add_assoc_zval(return_value, "right", &right);
	}
}

/* {{{ proto array typical_get_type(string $alias) */
ZEND_BEGIN_ARG_INFO_EX(typical_get_type_arginfo, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, alias)
ZEND_END_ARG_INFO();
static PHP_FUNCTION(typical_get_type) {
	zend_string *alias;
	php_typical_type_node *node;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(alias)
	ZEND_PARSE_PARAMETERS_END();

	node = typical_lookup_custom_type(alias);
	if (!node) {
		RETURN_FALSE;
	}

	typical_get_type(return_value, node);
} /* }}} */

static zend_function_entry typical_functions[] = {
	PHP_FE(typical_set_type, typical_set_type_arginfo)
	PHP_FE(typical_set_callback, typical_set_callback_arginfo)
	PHP_FE(typical_get_type, typical_get_type_arginfo)
	PHP_FE_END
};

#define TYPICAL_OPLINE const zend_op *opline = EX(opline)

static php_typical_type_node *typical_lookup_custom_type(const zend_string *type_name) {
	zend_string *lname;
	zval *type;

	if (!TYPICALG(types)) return NULL;
	lname = php_string_tolower((zend_string*)type_name);
	type = zend_hash_find(TYPICALG(types), lname);
	zend_string_release(lname);
	if (!type || (Z_TYPE_P(type) != IS_PTR)) {
		return NULL;
	}
	return (php_typical_type_node*)Z_PTR_P(type);
}

static int typical_verify_scalar_type(zend_uchar type, zval *val, zend_bool strict) {
	if (type == Z_TYPE_P(val)) return SUCCESS;

	if ((type == _IS_BOOL) &&
		((Z_TYPE_P(val) == IS_TRUE) || (Z_TYPE_P(val) == IS_FALSE))) {
		return SUCCESS;
	}
	if ((type == IS_DOUBLE) && (Z_TYPE_P(val) == IS_LONG)) {
		convert_to_double(val);
		return SUCCESS;
	}

	if (strict) {
		return FAILURE;
	}

	/* Else, weak typing */
	switch (type) {
		case _IS_BOOL: {
			zend_bool dest;
			if (!zend_parse_arg_bool_weak(val, &dest)) {
				return FAILURE;
			}
			zval_ptr_dtor(val);
			ZVAL_BOOL(val, dest);
			return SUCCESS;
		}
		case IS_LONG: {
			zend_long dest;
			if (!zend_parse_arg_long_weak(val, &dest)) {
				return FAILURE;
			}
			zval_ptr_dtor(val);
			ZVAL_LONG(val, dest);
			return SUCCESS;
		}
		case IS_DOUBLE: {
			double dest;
			if (!zend_parse_arg_double_weak(val, &dest)) {
				return FAILURE;
			}
			zval_ptr_dtor(val);
			ZVAL_DOUBLE(val, dest);
			return SUCCESS;
		}
		case IS_STRING: {
			zend_string *dest;
			/* on success "val" is converted to IS_STRING */
			if (!zend_parse_arg_str_weak(val, &dest)) {
				return FAILURE;
			}
			return SUCCESS;
		}
		default:
			return FAILURE;
	}
}

static int typical_verify_callback(zval *callback, zval *val, zend_bool strict) {
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval retval, args[2];
	int ret;

	if (UNEXPECTED(FAILURE == zend_fcall_info_init(callback, 0, &fci, &fcc, NULL, NULL))) {
		return FAILURE;
	}

	ZVAL_ZVAL(&args[0], val, 1, 0);
	ZVAL_BOOL(&args[1], strict);

	fci.retval = &retval;
	fci.params = args;
	fci.param_count = 2;
	fci.no_separation = 1;

	if (FAILURE == zend_call_function(&fci, &fcc) || EG(exception)) {
		zval_ptr_dtor(&args[0]);
		return FAILURE;
	}
	zval_ptr_dtor(&args[0]);
	zval_ptr_dtor(val);
	ZVAL_ZVAL(val, &retval, 0, 0);

	return SUCCESS;
}

static int typical_verify_match_type(zend_uchar type_hint, zend_string *class_name, zval *val, zend_bool strict) {
	switch (type_hint) {
		case _IS_BOOL: case IS_LONG: case IS_DOUBLE: case IS_STRING:
			return typical_verify_scalar_type(type_hint, val, strict);
		case IS_OBJECT:
			return typical_verify_type_name(class_name, val, strict);
		case IS_CALLABLE:
			return zend_is_callable(val, IS_CALLABLE_CHECK_SILENT, NULL) ? SUCCESS : FAILURE;
		case IS_ITERABLE:
			return zend_is_iterable(val) ? SUCCESS : FAILURE;
		case IS_ARRAY:
			return (Z_TYPE_P(val) == IS_ARRAY) ? SUCCESS : FAILURE;
		default:
			return FAILURE;
	}
}

static int typical_verify_custom_type(php_typical_type_node *type, zval *val, zend_bool strict) {
	if (Z_TYPE_P(val) == IS_NULL && (type->flags & PHP_TYPICAL_FLAG_ALLOW_NULL)) {
		/* Nullable type */
		return SUCCESS;
	}

	switch (type->join) {
		case PHP_TYPICAL_MATCH_TYPE:
			return typical_verify_match_type(type->u.type.type_hint, type->u.type.class_name, val, strict);

		case PHP_TYPICAL_CALLBACK:
			return typical_verify_callback(&type->u.callback, val, strict);

		case PHP_TYPICAL_JOIN_AND:
			return ((SUCCESS == typical_verify_custom_type(type->u.binary.left, val, strict)) &&
			        (SUCCESS == typical_verify_custom_type(type->u.binary.right, val, strict)))
			       ? SUCCESS : FAILURE;
		case PHP_TYPICAL_JOIN_OR:
			return ((SUCCESS == typical_verify_custom_type(type->u.binary.left, val, strict)) ||
			        (SUCCESS == typical_verify_custom_type(type->u.binary.right, val, strict)))
			       ? SUCCESS : FAILURE;
		case PHP_TYPICAL_JOIN_XOR:
			return ((SUCCESS == typical_verify_custom_type(type->u.binary.left, val, strict)) ^
			        (SUCCESS == typical_verify_custom_type(type->u.binary.right, val, strict)))
			       ? SUCCESS : FAILURE;
		default:
			return FAILURE;
	}
}

static int typical_verify_type_name(zend_string *type_name, zval *val, zend_bool strict) {
	php_typical_type_node *type = typical_lookup_custom_type(type_name);

	if (!type) {
		zend_class_entry *ce;
		if (Z_TYPE_P(val) != IS_OBJECT) return FAILURE;
		ce = zend_fetch_class(type_name, ZEND_FETCH_CLASS_AUTO | ZEND_FETCH_CLASS_NO_AUTOLOAD);
		return (ce && instanceof_function(Z_OBJCE_P(val), ce)) ? SUCCESS : FAILURE;
	}

	return typical_verify_custom_type(type, val, strict);
}

static int typical_verify_arg_type(zend_function *zf, uint32_t arg_num, zval *arg, void **cache_slot) {
	zend_arg_info *info;
	zend_bool strict = ZEND_ARG_USES_STRICT_TYPES();

	if (EXPECTED(arg_num <= zf->common.num_args)) {
		info = &zf->common.arg_info[arg_num-1];
	} else if (UNEXPECTED(zf->common.fn_flags & ZEND_ACC_VARIADIC)) {
		info = &zf->common.arg_info[zf->common.num_args];
	} else {
		return FAILURE;
	}

	if ((Z_TYPE_P(arg) == IS_NULL) && info->allow_null) {
		return SUCCESS;
	}

	ZVAL_DEREF(arg);
	if ((info->type_hint == IS_OBJECT) && info->class_name) {
		php_typical_type_node *type = typical_lookup_custom_type(info->class_name);
		if (type) {
			return typical_verify_custom_type(type, arg, strict);
		}
	}
	return typical_verify_match_type(info->type_hint, info->class_name, arg, strict);
}

static int typical_RECV(zend_execute_data *execute_data) {
	TYPICAL_OPLINE;
	uint32_t arg_num = opline->op1.num;
	zval *arg = EX_VAR(opline->result.var);
	void **slot = CACHE_ADDR(opline->op2.num);

	if (arg_num > EX_NUM_ARGS()) {
		return ZEND_USER_OPCODE_DISPATCH;
	}

	if ((typical_verify_arg_type(EX(func), arg_num, arg, slot) == FAILURE) &&
		!EG(exception)) {
		/* Will fail and raise TypeError */
		zend_check_arg_type(EX(func), arg_num, arg, NULL, slot);
	}

	++EX(opline);
	return ZEND_USER_OPCODE_CONTINUE;
}

static int typical_RECV_INIT(zend_execute_data *execute_data) {
	TYPICAL_OPLINE;
	uint32_t arg_num = opline->op1.num;
	zval *arg = EX_VAR(opline->result.var);
	zval *def = EX_CONSTANT(opline->op2);
	void **slot = CACHE_ADDR(Z_CACHE_SLOT_P(def));

	if (arg_num > EX_NUM_ARGS()) {
		ZVAL_COPY(arg, def);
		if (Z_OPT_CONSTANT_P(arg)) {
			if (UNEXPECTED(zval_update_constant_ex(arg, EX(func)->op_array.scope) == FAILURE)) {
				zval_ptr_dtor(arg);
				ZVAL_UNDEF(arg);
				++EX(opline);
				return ZEND_USER_OPCODE_CONTINUE;
			}
		}
	}

	if ((typical_verify_arg_type(EX(func), arg_num, arg, slot) == FAILURE) &&
		!EG(exception)) {
		/* Let the engine's type verifier raise the error for us */
		zend_check_arg_type(EX(func), arg_num, arg, def, slot);
	}

	++EX(opline);
	return ZEND_USER_OPCODE_CONTINUE;
}

static int typical_RECV_VARIADIC(zend_execute_data *execute_data) {
	TYPICAL_OPLINE;
	uint32_t arg_num = opline->op1.num;
	uint32_t arg_count = EX_NUM_ARGS();
	zval *arg, *params = EX_VAR(opline->result.var);

	if ((arg_num > arg_count) ||
		((EX(func)->op_array.fn_flags & ZEND_ACC_HAS_TYPE_HINTS) == 0)) {
		return ZEND_USER_OPCODE_DISPATCH;
	}

	array_init_size(params, arg_count - arg_num + 1);
	zend_hash_real_init(Z_ARRVAL_P(params), 1);
	ZEND_HASH_FILL_PACKED(Z_ARRVAL_P(params)) {
		void **slot = CACHE_ADDR(opline->op2.num);
		arg = EX_VAR_NUM(EX(func)->op_array.last_var + EX(func)->op_array.T);
		do {
			typical_verify_arg_type(EX(func), arg_num, arg, slot);
			if (Z_OPT_REFCOUNTED_P(arg)) Z_ADDREF_P(arg);
			ZEND_HASH_FILL_ADD(arg);
			++arg;
		} while (++arg_num <= arg_count);
	} ZEND_HASH_FILL_END();

	++EX(opline);
	return ZEND_USER_OPCODE_CONTINUE;
}

/* {{{ PHP_MINI_FUNCTION */
static PHP_MINIT_FUNCTION(typical) {
	REGISTER_LONG_CONSTANT("TYPICAL_FLAG_ALLOW_NULL", PHP_TYPICAL_FLAG_ALLOW_NULL, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("TYPICAL_MATCH_TYPE", PHP_TYPICAL_MATCH_TYPE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("TYPICAL_CALLBACK",   PHP_TYPICAL_CALLBACK,   CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("TYPICAL_JOIN_AND",   PHP_TYPICAL_JOIN_AND,   CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("TYPICAL_JOIN_OR",    PHP_TYPICAL_JOIN_OR,    CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("TYPICAL_JOIN_XOR",   PHP_TYPICAL_JOIN_XOR,   CONST_CS | CONST_PERSISTENT);
	return (
		(SUCCESS == zend_set_user_opcode_handler(ZEND_RECV, typical_RECV)) &&
		(SUCCESS == zend_set_user_opcode_handler(ZEND_RECV_INIT, typical_RECV_INIT)) &&
		(SUCCESS == zend_set_user_opcode_handler(ZEND_RECV_VARIADIC, typical_RECV_VARIADIC)))
	? SUCCESS : FAILURE;
} /* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION */
static PHP_RSHUTDOWN_FUNCTION(typical) {
	if (TYPICALG(types)) {
		zend_hash_destroy(TYPICALG(types));
		FREE_HASHTABLE(TYPICALG(types));
		TYPICALG(types) = NULL;
	}
} /* }}} */

/* {{{ PHP_GINIT_FUNCTION */
static PHP_GINIT_FUNCTION(typical) {
#if defined(COMPILE_DL_TYPICAL) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	typical_globals->types = NULL;
}
/* }}} */

/* {{{ typical_module_entry
 */
zend_module_entry typical_module_entry = {
	STANDARD_MODULE_HEADER,
	"typical",
	typical_functions,
	PHP_MINIT(typical),
	NULL, /* MSHUTDOWN */
	NULL, /* RINIT */
	PHP_RSHUTDOWN(typical),
	NULL, /* MINFO */
	"7.0.0-dev",
	PHP_MODULE_GLOBALS(typical),
	PHP_GINIT(typical),
	NULL, /* GSHUTDOWN */
	NULL, /* RPOSTSHUTDOWN */
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_TYPICAL
ZEND_GET_MODULE(typical)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
