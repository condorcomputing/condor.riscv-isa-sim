#=========================================================================
# Toplevel configure.ac for the Modular C++ Build System
#=========================================================================
# Please read the documentation in 'mcppbs-doc.txt' for more details on
# how the Modular C++ Build System works. For most new projects, a
# developer will only need to make the following changes:
#
#  - change the project metadata listed right below
#  - update the list of subprojects via the 'MCPPBS_SUBPROJECTS' macro
#  - possibly add subproject groups if needed to ease configuration
#  - add more configure checks for platform specific configuration
#

#-------------------------------------------------------------------------
# Project metadata
#-------------------------------------------------------------------------

m4_define( proj_name,         [RISC-V ISA Simulator])
m4_define( proj_maintainer,   [Andrew Waterman])
m4_define( proj_abbreviation, [spike])

#-------------------------------------------------------------------------
# Project version information
#-------------------------------------------------------------------------
# Version information is meant to be managed through a version control
# system's tags and revision numbers. In a working copy the version will
# not be defined here (you should just use the version control system's
# mechanisms). When we make a distribution then we can set the version
# here as formed by the scripts/vcs-version.sh script so that the
# distribution knows what version it came from. If you are not using
# version control then it is fine to set this directly.

m4_define( proj_version, [?])

#FIXME: next time
#m4_define([MAVIS_SUBMODULE_DIR], [jn.mavis])
#-------------------------------------------------------------------------
# Setup
#-------------------------------------------------------------------------

AC_INIT([proj_name],[proj_version],[proj_maintainer],[proj_abbreviation])
AC_LANG([C++])
AC_CONFIG_SRCDIR([riscv/common.h])
AC_CONFIG_AUX_DIR([scripts])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

m4_include(m4/ax_require_defined.m4)
m4_include(m4/ax_append_flag.m4)
m4_include(m4/ax_check_compile_flag.m4)
m4_include(m4/ax_check_link_flag.m4)
m4_include(m4/ax_append_link_flags.m4)
m4_include(m4/ax_boost_base.m4)
m4_include(m4/ax_boost_asio.m4)
m4_include(m4/ax_boost_regex.m4)

#-------------------------------------------------------------------------
# Handle --with-mavis
#-------------------------------------------------------------------------
AC_ARG_WITH([mavis],
  AS_HELP_STRING([--with-mavis=PATH], [Use external MAVIS installation at PATH]),
  [external_mavis_path="$withval"],
  [external_mavis_path=""]
)
AC_SUBST([EXTERNAL_MAVIS_PATH], [$external_mavis_path])

AC_MSG_NOTICE([DEBUG: external_mavis_path = $external_mavis_path])

#-------------------------------------------------------------------------
# Handle --with-stf-lib
#-------------------------------------------------------------------------
AC_ARG_WITH([stf-lib],
  AS_HELP_STRING([--with-stf-lib=PATH], [Use external STF_LIB installation at PATH]),
  [external_stf_lib_path="$withval"],
  [external_stf_lib_path=""]
)
AC_SUBST([EXTERNAL_STF_LIB_PATH], [$external_stf_lib_path])

#-------------------------------------------------------------------------
# Checks for programs
#-------------------------------------------------------------------------

AC_PROG_CC
AC_PROG_CXX
AC_CHECK_TOOL([AR],[ar])
AC_CHECK_TOOL([RANLIB],[ranlib])
AC_PATH_PROG([DTC],[dtc],[no])
AS_IF([test x"$DTC" == xno],AC_MSG_ERROR([device-tree-compiler not found]))
AC_DEFINE_UNQUOTED(DTC, ["dtc"], [Executable name of device-tree-compiler])

AC_C_BIGENDIAN

#-------------------------------------------------------------------------
# Check for riscv64-unknown-elf-gcc
#-------------------------------------------------------------------------

AC_PATH_PROG([RISCV_GCC], [riscv64-unknown-elf-gcc], [no])

if test "x$RISCV_GCC" = "xno"; then
    AC_MSG_ERROR([riscv64-unknown-elf-gcc not found in PATH. Please install the RISC-V toolchain and ensure it is accessible.])
else
    AC_MSG_NOTICE([Found riscv64-unknown-elf-gcc: $RISCV_GCC])
fi

#-------------------------------------------------------------------------
# Check for the stf_lib submodule and extract Git SHA
#-------------------------------------------------------------------------

AC_MSG_CHECKING([for stf_lib submodule])

if test -d "${srcdir}/stf_lib" && test -f "${srcdir}/stf_lib/.git"; then
    AC_MSG_RESULT([found])
else
    AC_MSG_NOTICE([stf_lib submodule not found or incomplete. Initializing and updating...])
    if ! (cd "${srcdir}" && git submodule update --init --recursive); then
        AC_MSG_ERROR([Failed to initialize and update stf_lib submodule. Ensure git is available and the submodule is correctly configured.])
    fi
fi

# Extract Git SHA for STF_SPIKE and stf_lib
STF_SPIKE_GIT_SHA=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
AC_SUBST([STF_SPIKE_GIT_SHA], [$STF_SPIKE_GIT_SHA])

if test -d "${srcdir}/stf_lib"; then
    STF_LIB_GIT_SHA=$(git -C ${srcdir}/stf_lib rev-parse HEAD 2>/dev/null || echo "unknown")
else
    STF_LIB_GIT_SHA="unknown"
fi
AC_SUBST([STF_LIB_GIT_SHA], [$STF_LIB_GIT_SHA])

#-------------------------------------------------------------------------
#-------------------------------------------------------------------------
AS_IF([test "x$external_mavis_path" = "x"], [
  AC_MSG_CHECKING([for jn.mavis submodule])
  if test -d "${srcdir}/jn.mavis" && test -f "${srcdir}/jn.mavis/.git"; then
      AC_MSG_RESULT([found])
  else
      AC_MSG_NOTICE([jn.mavis submodule not found or incomplete. Initializing and updating...])
      if ! (cd "${srcdir}" && git submodule update --init --recursive); then
          AC_MSG_ERROR([Failed to initialize and update jn.mavis submodule.])
      fi
  fi

  MAVIS_GIT_SHA=$(git -C ${srcdir}/jn.mavis rev-parse HEAD 2>/dev/null || echo "unknown")
  AC_SUBST([MAVIS_GIT_SHA], [$MAVIS_GIT_SHA])

  AC_MSG_NOTICE([Configuring and building jn.mavis submodule])
  MAVIS_BUILD_DIR="${srcdir}/jn.mavis/release"
  if test ! -d "$MAVIS_BUILD_DIR"; then
      mkdir -p "$MAVIS_BUILD_DIR" || AC_MSG_ERROR([Failed to create MAVIS build directory])
  fi
  (cd "$MAVIS_BUILD_DIR" && cmake .. -DCMAKE_BUILD_TYPE=FastDebug -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF && make -j$(nproc)) \
      || AC_MSG_ERROR([Failed to build jn.mavis submodule])
], [
  AC_MSG_NOTICE([Using external MAVIS at $external_mavis_path])
])

#-------------------------------------------------------------------------
# Get current repo SHA
#-------------------------------------------------------------------------
GIT_SHA=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
AC_SUBST([GIT_SHA])

AS_IF([test "x$external_stf_lib_path" = "x"], [
  AC_MSG_NOTICE([Configuring and building stf_lib submodule])
  STF_LIB_BUILD_DIR="${srcdir}/stf_lib/release"
  if test ! -d "$STF_LIB_BUILD_DIR"; then
      mkdir -p "$STF_LIB_BUILD_DIR" || AC_MSG_ERROR([Failed to create build directory for stf_lib])
  fi

  (cd "$STF_LIB_BUILD_DIR" && cmake .. -DCMAKE_BUILD_TYPE=FastDebug -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF && make -j$(nproc)) || \
      AC_MSG_ERROR([Failed to build stf_lib submodule])

  STF_LIB_PATH="${STF_LIB_BUILD_DIR}/lib"
], [
  AC_MSG_NOTICE([Using external STF_LIB at $external_stf_lib_path])
  STF_LIB_PATH="${external_stf_lib_path}/lib"
])
AC_SUBST([STF_LIB_PATH])


#-------------------------------------------------------------------------
# Add stf_lib to linker flags
#-------------------------------------------------------------------------

STF_LIB_PATH="${srcdir}/stf_lib/release/lib"
if test -f "${STF_LIB_PATH}/libstf.a"; then
    AC_MSG_NOTICE([Found libstf.a at ${STF_LIB_PATH}])
    LDFLAGS="-L${STF_LIB_PATH} $LDFLAGS"
    LIBS="-lstf $LIBS"
else
    AC_MSG_ERROR([libstf.a not found in ${STF_LIB_PATH}. Ensure stf_lib is built correctly.])
fi

#-------------------------------------------------------------------------
# Check for the dhrystone directory
#-------------------------------------------------------------------------
AC_MSG_CHECKING([for dhrystone directory])

DHRYSTONE_PATH="../dhrystone"

if test -d "$DHRYSTONE_PATH" && test -f "$DHRYSTONE_PATH/Makefile"; then
    AC_MSG_RESULT([found at $DHRYSTONE_PATH])
else
    AC_MSG_ERROR([dhrystone directory not found or incomplete. Ensure it is present in ../dhrystone])
fi

AC_SUBST([DHRYSTONE_PATH], [$DHRYSTONE_PATH])
AC_SUBST([DHRYSTONE_INSTALL_BIN], [$prefix/bin])

#-------------------------------------------------------------------------
# Version Information
#-------------------------------------------------------------------------
STF_SPIKE_VERSION_MAJOR=1
STF_SPIKE_VERSION_MINOR=0
STF_SPIKE_VERSION_PATCH=0
AC_SUBST([STF_SPIKE_VERSION_MAJOR])
AC_SUBST([STF_SPIKE_VERSION_MINOR])
AC_SUBST([STF_SPIKE_VERSION_PATCH])

#-------------------------------------------------------------------------
# Check for the riscv-tests submodule
#-------------------------------------------------------------------------

AC_MSG_CHECKING([for riscv-tests submodule])

if test -d "${srcdir}/riscv-tests" && test -f "${srcdir}/riscv-tests/.git"; then
    AC_MSG_RESULT([found])
else
    AC_MSG_NOTICE([riscv-tests submodule not found or incomplete. Initializing and updating...])
    if ! (cd "${srcdir}" && git submodule update --init --recursive); then
        AC_MSG_ERROR([Failed to initialize and update riscv-tests submodule. Ensure git is available and the submodule is correctly configured.])
    fi
fi

#-------------------------------------------------------------------------
# Configure and build riscv-tests submodule
#-------------------------------------------------------------------------

AC_MSG_NOTICE([Configuring and building riscv-tests submodule])
RISCV_TESTS_DIR="${srcdir}/riscv-tests"

# Ensure riscv-tests is fully checked out
if test ! -d "$RISCV_TESTS_DIR" || test ! -f "$RISCV_TESTS_DIR/configure.ac"; then
    AC_MSG_ERROR([riscv-tests submodule is missing or incomplete. Ensure it is properly cloned and submodules are updated.])
fi

# Run autoconf if configure script does not exist
if test ! -f "$RISCV_TESTS_DIR/configure"; then
    (cd "$RISCV_TESTS_DIR" && autoconf) || AC_MSG_ERROR([Failed to run autoconf for riscv-tests])
fi

# Set the correct install path relative to the build directory
INSTALL_DIR="$(pwd)/../install"

# Configure, build, and install riscv-tests
if test -f "$RISCV_TESTS_DIR/configure"; then
    (cd "$RISCV_TESTS_DIR" && \
        ./configure --prefix="$INSTALL_DIR" && \
        make -j$(nproc) && \
        make install) || AC_MSG_ERROR([Failed to build and install riscv-tests submodule])
else
    AC_MSG_ERROR([riscv-tests configure script not found after autoconf])
fi

#-------------------------------------------------------------------------
# These macros check to see if we can do a stow-based install and also
# check for an isa simulator suitable for running the unit test programs
# via the makefile.

MCPPBS_PROG_INSTALL

#-------------------------------------------------------------------------
# Checks for type
#-------------------------------------------------------------------------

AC_CHECK_TYPE([__int128_t],
              [AC_SUBST([HAVE_INT128],[yes])
              AC_DEFINE([HAVE_INT128], [1], [__int128_t is supported])])

#-------------------------------------------------------------------------
# Default compiler flags
#-------------------------------------------------------------------------

AX_APPEND_LINK_FLAGS([-Wl,--export-dynamic])

AX_CHECK_COMPILE_FLAG([-relocatable-pch], AC_SUBST([HAVE_CLANG_PCH],[yes]))

#-------------------------------------------------------------------------
# MCPPBS subproject list
#-------------------------------------------------------------------------
# Order list so that subprojects only depend on those listed earlier.
# The '*' suffix indicates an optional subproject. The '**' suffix
# indicates an optional subproject which is also the name of a group.

MCPPBS_SUBPROJECTS([ fesvr, riscv, disasm, customext, fdt, softfloat, spike_main, spike_dasm ])

#-------------------------------------------------------------------------
# MCPPBS subproject groups
#-------------------------------------------------------------------------
# If a group has the same name as a subproject then you must add the
# '**' suffix in the subproject list above. The list of subprojects in a
# group should be ordered so that subprojets only depend on those listed
# earlier. Here is an example:
#
#  MCPPBS_GROUP( [group-name], [sproja,sprojb,...] )
#

#-------------------------------------------------------------------------
# Output
#-------------------------------------------------------------------------

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([riscv-fesvr.pc])
AC_CONFIG_FILES([riscv-disasm.pc])
AC_CONFIG_FILES([riscv-riscv.pc])

#-------------------------------------------------------------------------
AC_MSG_NOTICE([================================================])
AC_MSG_NOTICE([ Configuration Summary ])
AC_MSG_NOTICE([================================================])
AS_IF([test "x$external_mavis_path" = "x"], [
  AC_MSG_NOTICE([ MAVIS        : building local submodule])
], [
  AC_MSG_NOTICE([ MAVIS        : external at $external_mavis_path])
])

AS_IF([test "x$external_stf_lib_path" = "x"], [
  AC_MSG_NOTICE([ STF_LIB      : building local submodule])
], [
  AC_MSG_NOTICE([ STF_LIB      : external at $external_stf_lib_path])
])

#AS_IF([test "x$external_stf_tools_path" = "x"], [
#  AC_MSG_NOTICE([ STF_TOOLS    : building local submodule])
#], [
#  AC_MSG_NOTICE([ STF_TOOLS    : external at $external_stf_tools_path])
#])

AC_MSG_NOTICE([ riscv-tests  : building local submodule])
AC_MSG_NOTICE([================================================])

#-------------------------------------------------------------------------

AC_OUTPUT
