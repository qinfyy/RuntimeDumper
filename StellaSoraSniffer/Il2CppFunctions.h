#pragma once
#include "il2cpp-class-internals.h"

#define DO_API(r, n, p) inline r (*n) p = nullptr;
#define DO_API_NO_RETURN(r, n, p) inline r (*n) p = nullptr;
#include "il2cpp-api-functions.h"
#undef DO_API
#undef DO_API_NO_RETURN


void InitIl2CppFunctions();

bool AttachIl2Cpp(Il2CppDomain*& outDomain, Il2CppThread*& outThread);

void PrintAllImageNames();