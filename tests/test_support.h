#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include "sgp4_bridge.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        std::exit(1); \
    } \
} while (0)

struct ElementCase {
    std::string id;
    Sgp4Elements elements;
};

std::vector<ElementCase> read_elements(const std::string &path);
void test_modes();
extern "C" int test_modes_c(const Sgp4Elements *elements);
extern "C" int test_api();
extern "C" int test_propagation_failure(const Sgp4Elements *elements, char mode,
    double time, int expected_error, const char *case_id, int dump);

#endif
