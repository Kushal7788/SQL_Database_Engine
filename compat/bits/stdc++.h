// Portable stand-in for GCC's <bits/stdc++.h>.
//
// The sources include <bits/stdc++.h>, which ships with libstdc++ (GCC) but
// not with libc++ (Apple Clang). This header is NOT used by the default
// build. To compile with Clang (e.g. on macOS), add this directory to the
// include path:
//
//   cd src && make CXX=clang++ CXXFLAGS="-g -I . -I ../compat"
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
