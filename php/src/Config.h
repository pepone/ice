//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_PHP_CONFIG_H
#define ICE_PHP_CONFIG_H

//
// We need to define WIN32_LEAN_AND_MEAN to avoid redefinition errors in
// winsock2.h. However, we can't define the macro in the Makefile because
// a PHP header defines it without a surrounding #ifndef, so we have to
// undefine it before including the PHP header files.
//
#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#endif

#include <Ice/Ice.h>

#ifdef _WIN32
#   undef WIN32_LEAN_AND_MEAN
#endif

#ifdef _WIN32
#include <crtdbg.h>
#include <math.h>
#endif

#ifdef _WIN32
extern "C"
{
#endif

#ifdef _WIN32
#   pragma warning( disable : 4018) // suppress signed/unsigned mismatch in zend_execute.h (PHP 5.3.x)
#elif defined(__clang__)
    // Surpress various warnings emitted from including the PHP headers
#   pragma clang diagnostic ignored "-Wconversion"
#   pragma clang diagnostic ignored "-Wsign-conversion"
#   pragma clang diagnostic ignored "-Wdocumentation"
#   pragma clang diagnostic ignored "-Wshadow"
#elif defined(__GNUC__)
#   pragma GCC diagnostic warning "-Wsign-compare"
#endif

#ifdef _WIN64
#   pragma warning( disable : 4267) // suppress size_t/uint conversion warnings in zend macros for Windows x64 builds
#endif

#if defined(__GNUC__)
#   pragma GCC diagnostic warning "-Wnarrowing"
#   pragma GCC diagnostic ignored "-Wignored-qualifiers"
#   pragma GCC diagnostic ignored "-Wredundant-decls"
#endif

//
// The php.h header defines/undefines NDEBUG based on how the PHP binary was built.
// As a result, asserts are always disabled unless building against a php binary
// built with --enable-debug. We want to enable asserts for the PHP Ice extension
// when it's built without OPTIMIZE=yes. We save NDEBUG in a tmp macro here and
// explicitly re-include the assert.h header with the saved NDEBUG macro after
// including php.h
//
#ifndef NDEBUG
#define TMPDEBUG
#endif

#include "php.h"

#ifdef _WIN32
#   pragma warning( default : 4018)
#endif

#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"

#ifdef _WIN32
}
#endif

//
// Enable asserts if the extension is built with debug. It's fine to include several times
// assert.h with a different NDEBUG setting.
//
#ifdef TMPDEBUG
#undef TMPDEBUG
#undef NDEBUG
#include <assert.h>
#endif

extern zend_module_entry ice_module_entry;
#define phpext_ice_ptr &ice_module_entry

#ifdef PHP_WIN32
#define PHP_ICE_API __declspec(dllexport)
#else
#define PHP_ICE_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

ZEND_MINIT_FUNCTION(ice);
ZEND_MSHUTDOWN_FUNCTION(ice);
ZEND_RINIT_FUNCTION(ice);
ZEND_RSHUTDOWN_FUNCTION(ice);
ZEND_MINFO_FUNCTION(ice);

ZEND_BEGIN_MODULE_GLOBALS(ice)
    void* communicatorMap;
    void* idToClassInfoMap;
    void* compactIdToClassInfoMap;
    void* nameToClassInfoMap;
    void* proxyInfoMap;
    void* exceptionInfoMap;
    zval* unset;
ZEND_END_MODULE_GLOBALS(ice)

// A void arginfo used for methods which do not take parameters.
ZEND_BEGIN_ARG_INFO(ice_void_arginfo, 0)
ZEND_END_ARG_INFO()

// An arginfo used for __toString() methods.
#if PHP_VERSION_ID >= 80200
 ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO(ice_to_string_arginfo, IS_STRING, 0)
 ZEND_END_ARG_INFO()
#else
// The return type declaration is optional with PHP < 8.2
ZEND_BEGIN_ARG_INFO(ice_to_string_arginfo, 0)
ZEND_END_ARG_INFO()
#endif
#ifdef ZTS
#   define ICE_G(v) TSRMG(ice_globals_id, zend_ice_globals*, v)
#else
#   define ICE_G(v) (ice_globals.v)
#endif

#ifndef Z_ADDREF_P
#   ifndef ZVAL_ADDREF
#       error "Unknown PHP version"
#   endif
#   define Z_ADDREF_P(zv) ZVAL_ADDREF(zv)
#endif

#ifndef ZEND_MN
#   define ZEND_MN(name) ZEND_FN(name)
#endif

//
// Older versions of PHP use char* instead of const char* in many APIs.
//
#ifdef STRCAST
#   error "STRCAST already defined!"
#endif
#define STRCAST(s) const_cast<char*>(s)

// PHP8 removed TSRMLS_CC, however older versions still require it
#ifndef TSRMLS_CC
#   define TSRMLS_CC
#endif

#endif
