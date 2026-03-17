#pragma once
#pragma warning(push)
#pragma warning(disable: 4244 4273 4244)
#define HAVE_ROUND
#define HAVE_HYPOT
// Qt defines 'slots' as empty macro; Python 3.12 object.h uses 'slots' as a struct member name.
// Undefine before including Python, restore after to avoid C2059 syntax errors.
#ifdef slots
#undef slots
#define _VICE_RESTORE_SLOTS 1
#endif
#include <boost/python.hpp>
#ifdef _VICE_RESTORE_SLOTS
#define slots
#undef _VICE_RESTORE_SLOTS
#endif
#pragma warning(pop)
// Python 2→3 compat: BOOST_PYTHON_MODULE(Vice) now generates PyInit_Vice, not initVice
#if PY_MAJOR_VERSION >= 3
#define initVice PyInit_Vice
#endif
