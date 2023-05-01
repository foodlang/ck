#!/bin/bash
premake5/linux gmake2 config=Debug
make -j 8 config=debug_x86_64