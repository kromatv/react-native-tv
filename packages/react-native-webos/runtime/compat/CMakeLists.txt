cmake_minimum_required(VERSION 3.24)
project(rn_webos_glibc_compat C)

add_library(rn-webos-glibc-compat STATIC glibc-compat.c)
target_compile_options(rn-webos-glibc-compat PRIVATE -O2 -Wall -Wextra -Werror)
