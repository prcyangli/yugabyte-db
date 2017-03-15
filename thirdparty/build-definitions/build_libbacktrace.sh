# Copyright (c) YugaByte, Inc.

LIBBACKTRACE_VERSION=ba79a27ee9a62b1be86d0ddae7614c316b7f6fbb
LIBBACKTRACE_URL="https://github.com/yugabyte/libbacktrace/archive/$LIBBACKTRACE_VERSION.zip"
LIBBACKTRACE_DIR=$TP_SOURCE_DIR/libbacktrace-$LIBBACKTRACE_VERSION

build_libbacktrace() {
  local yb_skip_build_dir_cleanup=1
  create_build_dir_and_prepare "$LIBBACKTRACE_DIR"
  (
    set build_env_vars
    set -x
    CFLAGS="$EXTRA_CFLAGS" \
      CXXFLAGS="$EXTRA_CXXFLAGS" \
      ./configure --with-pic --prefix=$PREFIX
    run_make install
  )
}
