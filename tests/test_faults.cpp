/* Isolated failure injection: link the real bridge against controlled SGP4
 * stubs, never into the production archive or the numerical test executable. */
#include "SGP4.h"
#include "test_support.h"
#include <new>

static bool fail_allocation = false;
static bool init_success = true;
static bool predict_success = true;
static int reported_error = 0;

void *operator new(std::size_t size, const std::nothrow_t &) noexcept
{
    return fail_allocation ? nullptr : ::operator new(size);
}

namespace SGP4Funcs {
bool sgp4init(gravconsttype, char, const char[6], double, double, double,
              double, double, double, double, double, double, double,
              elsetrec &record)
{
    record.error = reported_error;
    return init_success;
}

bool sgp4(elsetrec &record, double, double r[3], double v[3])
{
    record.error = reported_error;
    for (int i = 0; i < 3; ++i) r[i] = v[i] = 999;
    return predict_success;
}
}

int main()
{
    const Sgp4Elements elements = {"12345", 20000.0, 1e-5, 0.0, 0.0,
        0.01, 0.5, 0.9, 0.2, 0.067, 1.0};
    const char modes[] = {'a', 'i'};
    for (char mode : modes) {
        int err = 99;
        fail_allocation = true;
        CHECK(sgp4_create(&elements, mode, &err) == nullptr && err == -2);
        fail_allocation = false;
        init_success = false;
        reported_error = 0;
        CHECK(sgp4_create(&elements, mode, &err) == nullptr && err == -3);
        for (int error = 1; error <= 6; ++error) {
            reported_error = error;
            CHECK(sgp4_create(&elements, mode, &err) == nullptr && err == error);
            init_success = true;
            CHECK(sgp4_create(&elements, mode, &err) == nullptr && err == error);
            init_success = false;
        }
        init_success = true;
        reported_error = 0;
        Sgp4Satellite *sat = sgp4_create(&elements, mode, &err);
        CHECK(sat != nullptr && err == 0);
        predict_success = false;
        for (int error = 0; error <= 6; ++error) {
            reported_error = error;
            Sgp4Vector p = {11, 22, 33}, v = {44, 55, 66};
            CHECK(sgp4_predict(sat, 0, &p, &v) == (error ? error : -3));
            CHECK(p.x == 11 && p.y == 22 && p.z == 33);
            CHECK(v.x == 44 && v.y == 55 && v.z == 66);
        }
        sgp4_destroy(sat);
        reported_error = 0;
    }
    std::puts("Allocation and SGP4 failure injection tests passed");
}
