cmake_minimum_required(VERSION 3.24)
project(rn_tizen_glibc_compat C)

add_library(rn-tizen-glibc-compat STATIC glibc-compat.c)
target_compile_options(rn-tizen-glibc-compat PRIVATE -O2 -Wall -Wextra -Werror)
