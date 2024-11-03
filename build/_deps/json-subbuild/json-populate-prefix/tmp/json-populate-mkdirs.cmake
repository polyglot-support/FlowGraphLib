# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/runner/workspace/build/_deps/json-src"
  "/home/runner/workspace/build/_deps/json-build"
  "/home/runner/workspace/build/_deps/json-subbuild/json-populate-prefix"
  "/home/runner/workspace/build/_deps/json-subbuild/json-populate-prefix/tmp"
  "/home/runner/workspace/build/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp"
  "/home/runner/workspace/build/_deps/json-subbuild/json-populate-prefix/src"
  "/home/runner/workspace/build/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/runner/workspace/build/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/runner/workspace/build/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
