////////////////////////////////////////////////////////////////////////////////
//                                    jflags
//
// This file is distributed under the 3-clause Berkeley Software Distribution
// License. See LICENSE.txt for details.
////////////////////////////////////////////////////////////////////////////////
#ifndef JFLAGS_DEFINE_H_
#define JFLAGS_DEFINE_H_

#include "jflags_declare.h" // IWYU pragma: export

// We always want to export variables defined in user code
#ifndef JFLAGS_DLL_DEFINE_FLAG
#ifdef _MSC_VER
#define JFLAGS_DLL_DEFINE_FLAG __declspec(dllexport)
#else
#define JFLAGS_DLL_DEFINE_FLAG
#endif
#endif

namespace JFLAGS_NAMESPACE {

// --------------------------------------------------------------------
// Now come the command line flag declaration/definition macros that
// will actually be used.  They're kind of hairy.  A major reason
// for this is initialization: we want people to be able to access
// variables in global constructors and have that not crash, even if
// their global constructor runs before the global constructor here.
// (Obviously, we can't guarantee the flags will have the correct
// default value in that case, but at least accessing them is safe.)
// The only way to do that is have flags point to a static buffer.
// So we make one, using a union to ensure proper alignment, and
// then use placement-new to actually set up the flag with the
// correct default value.  In the same vein, we have to worry about
// flag access in global destructors, so FlagRegisterer has to be
// careful never to destroy the flag-values it constructs.
//
// Note that when we define a flag variable FLAGS_<name>, we also
// preemptively define a junk variable, FLAGS_no<name>.  This is to
// cause a link-time error if someone tries to define 2 flags with
// names like "logging" and "nologging".  We do this because a bool
// flag FLAG can be set from the command line to true with a "-FLAG"
// argument, and to false with a "-noFLAG" argument, and so this can
// potentially avert confusion.
//
// We also put flags into their own namespace.  It is purposefully
// named in an opaque way that people should have trouble typing
// directly.  The idea is that DEFINE puts the flag in the weird
// namespace, and DECLARE imports the flag from there into the current
// namespace.  The net result is to force people to use DECLARE to get
// access to a flag, rather than saying "extern JFLAGS_DLL_DECL bool
// FLAGS_whatever;"
// or some such instead.  We want this so we can put extra
// functionality (like sanity-checking) in DECLARE if we want, and
// make sure it is picked up everywhere.
//
// We also put the type of the variable in the namespace, so that
// people can't DECLARE_int32 something that they DEFINE_bool'd
// elsewhere.

// If your application #defines STRIP_FLAG_HELP to a non-zero value
// before #including this file, we remove the help message from the
// binary file. This can reduce the size of the resulting binary
// somewhat, and may also be useful for security reasons.

extern JFLAGS_DLL_DECL const char kStrippedFlagHelp[];

} // namespace JFLAGS_NAMESPACE

#ifndef SWIG // In swig, ignore the main flag declarations

#if defined(STRIP_FLAG_HELP) && STRIP_FLAG_HELP > 0
// Need this construct to avoid the 'defined but not used' warning.
#define MAYBE_STRIPPED_HELP(txt) \
    (false ? (txt) : JFLAGS_NAMESPACE::kStrippedFlagHelp)
#else
#define MAYBE_STRIPPED_HELP(txt) txt
#endif

// Each command-line flag has two variables associated with it: one
// with the current value, and one with the default value.  However,
// we have a third variable, which is where value is assigned; it's a
// constant.  This guarantees that FLAG_##value is initialized at
// static initialization time (e.g. before program-start) rather than
// than global construction time (which is after program-start but
// before main), at least when 'value' is a compile-time constant.  We
// use a small trick for the "default value" variable, and call it
// FLAGS_no<name>.  This serves the second purpose of assuring a
// compile error if someone tries to define a flag named no<name>
// which is illegal (--foo and --nofoo both affect the "foo" flag).
#define DEFINE_VARIABLE(type, shorttype, name, value, help)                 \
    namespace fL##shorttype                                                 \
    {                                                                       \
        static const type FLAGS_nono##name = value;                         \
        /* We always want to export defined variables, dll or no */         \
        JFLAGS_DLL_DEFINE_FLAG type FLAGS_##name = FLAGS_nono##name;        \
        type FLAGS_no##name = FLAGS_nono##name;                             \
        static JFLAGS_NAMESPACE::FlagRegisterer o_##name(                   \
          #name, #type, MAYBE_STRIPPED_HELP(help), __FILE__, &FLAGS_##name, \
          &FLAGS_no##name);                                                 \
    }                                                                       \
    using fL##shorttype::FLAGS_##name

// For DEFINE_bool, we want to do the extra check that the passed-in
// value is actually a bool, and not a string or something that can be
// coerced to a bool.  These declarations (no definition needed!) will
// help us do that, and never evaluate From, which is important.
// We'll use 'sizeof(IsBool(val))' to distinguish. This code requires
// that the compiler have different sizes for bool & double. Since
// this is not guaranteed by the standard, we check it with a
// COMPILE_ASSERT.
namespace fLB {
struct CompileAssert
{
};
typedef CompileAssert expected_sizeof_double_neq_sizeof_bool[(sizeof(double) != sizeof(bool)) ? 1 : -1];
template <typename From>
double JFLAGS_DLL_DECL IsBoolFlag(const From & from);
JFLAGS_DLL_DECL bool IsBoolFlag(bool from);
} // namespace fLB

// Here are the actual DEFINE_*-macros. The respective DECLARE_*-macros
// are in a separate include, jflags_declare.h, for reducing
// the physical transitive size for DECLARE use.
#define DEFINE_bool(name, val, txt)                                  \
    namespace fLB {                                                  \
    typedef ::fLB::CompileAssert FLAG_##name##_value_is_not_a_bool   \
      [(sizeof(::fLB::IsBoolFlag(val)) != sizeof(double)) ? 1 : -1]; \
    }                                                                \
    DEFINE_VARIABLE(bool, B, name, val, txt)

#define DEFINE_int32(name, val, txt) \
    DEFINE_VARIABLE(JFLAGS_NAMESPACE::int32, I, name, val, txt)

#define DEFINE_uint32(name, val, txt) \
    DEFINE_VARIABLE(JFLAGS_NAMESPACE::uint32, U, name, val, txt)

#define DEFINE_int64(name, val, txt) \
    DEFINE_VARIABLE(JFLAGS_NAMESPACE::int64, I64, name, val, txt)

#define DEFINE_uint64(name, val, txt) \
    DEFINE_VARIABLE(JFLAGS_NAMESPACE::uint64, U64, name, val, txt)

#define DEFINE_double(name, val, txt) DEFINE_VARIABLE(double, D, name, val, txt)

// Strings are trickier, because they're not a POD, so we can't
// construct them at static-initialization time (instead they get
// constructed at global-constructor time, which is much later).  To
// try to avoid crashes in that case, we use a char buffer to store
// the string, which we can static-initialize, and then placement-new
// into it later.  It's not perfect, but the best we can do.

namespace fLS {

inline clstring * dont_pass0toDEFINE_string(char * stringspot, const char * value)
{
    return new (stringspot) clstring(value);
}
inline clstring * dont_pass0toDEFINE_string(char * stringspot, const clstring & value)
{
    return new (stringspot) clstring(value);
}
inline clstring * dont_pass0toDEFINE_string(char * stringspot, int value);

// Auxiliary class used to explicitly call destructor of string objects
// allocated using placement new during static program deinitialization.
// The destructor MUST be an inline function such that the explicit
// destruction occurs in the same compilation unit as the placement new.
class StringFlagDestructor
{
    void * current_storage_;
    void * defvalue_storage_;

public:
    StringFlagDestructor(void * current, void * defvalue)
    : current_storage_(current), defvalue_storage_(defvalue) {}

    ~StringFlagDestructor()
    {
        reinterpret_cast<clstring *>(current_storage_)->~clstring();
        reinterpret_cast<clstring *>(defvalue_storage_)->~clstring();
    }
};

} // namespace fLS

// We need to define a var named FLAGS_no##name so people don't define
// --string and --nostring.  And we need a temporary place to put val
// so we don't have to evaluate it twice.  Two great needs that go
// great together!
// The weird 'using' + 'extern' inside the fLS namespace is to work around
// an unknown compiler bug/issue with the gcc 4.2.1 on SUSE 10.  See
//    http://code.google.com/p/google-jflags/issues/detail?id=20
#define DEFINE_string(name, val, txt)                                                       \
    namespace fLS {                                                                         \
    using ::fLS::clstring;                                                                  \
    static union                                                                            \
    {                                                                                       \
        void * align;                                                                       \
        char s[sizeof(clstring)];                                                           \
    } s_##name[2];                                                                          \
    clstring * const FLAGS_no##name = ::fLS::dont_pass0toDEFINE_string(s_##name[0].s, val); \
    static JFLAGS_NAMESPACE::FlagRegisterer                                                 \
      o_##name(#name, "string", MAYBE_STRIPPED_HELP(txt), __FILE__,                         \
               s_##name[0].s, new (s_##name[1].s) clstring(*FLAGS_no##name));               \
    static StringFlagDestructor d_##name(s_##name[0].s, s_##name[1].s);                     \
    extern JFLAGS_DLL_DEFINE_FLAG clstring & FLAGS_##name;                                  \
    using fLS::FLAGS_##name;                                                                \
    clstring & FLAGS_##name = *FLAGS_no##name;                                              \
    }                                                                                       \
    using fLS::FLAGS_##name

#endif // SWIG

#endif // JFLAGS_DEFINE_H_

