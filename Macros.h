#pragma once

// Declare a variable DEBUG_MUTABLE, define it RELEASE_CONST to achieve
// A static value when building in_DEBUG, thus making the variable changeable
// A const value in release builds, increasing release speed.

#ifdef _DEBUG
#define DEBUG_MUTABLE static
#define RELEASE_CONST
#define RELEASE_STATEMENT(...) ;

#else 
#define DEBUG_MUTABLE static const
#define RELEASE_CONST const
#define RELEASE_STATEMENT(...) __VA_ARGS__;
#endif
