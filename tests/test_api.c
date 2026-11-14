#include "sgp4_bridge.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); return 1; \
} } while (0)

static int invalid(const Sgp4Elements *elements, char mode)
{
    int err = 99;
    Sgp4Satellite *sat = sgp4_create(elements, mode, &err);
    if (sat != NULL) {
        sgp4_destroy(sat);
        return 1;
    }
    return err != -1;
}

int test_propagation_failure(const Sgp4Elements *elements, char mode,
    double time, int expected_error, const char *case_id, int dump)
{
    int err = 99;
    Sgp4Satellite *sat = sgp4_create(elements, mode, &err);
    Sgp4Vector p = {11, 22, 33}, v = {44, 55, 66};
    CHECK(sat != NULL && err == 0);
    CHECK(sgp4_predict(sat, time, &p, &v) == expected_error);
    CHECK(p.x == 11 && p.y == 22 && p.z == 33);
    CHECK(v.x == 44 && v.y == 55 && v.z == 66);
    if (dump) {
        printf("STATE,%s,%c,%.17g,%d,11,22,33,44,55,66\n",
               case_id, mode, time, expected_error);
    }
    /* A later valid request must reset the underlying error state. */
    CHECK(sgp4_predict(sat, 0, &p, &v) == 0);
    sgp4_destroy(sat);
    return 0;
}

int test_api(void)
{
    const Sgp4Elements base = {"12345", 20000.0, 1e-5, 0.0, 0.0,
        0.01, 0.5, 0.9, 0.2, 0.067, 1.0};
    const size_t offsets[] = {
        offsetof(Sgp4Elements, epoch), offsetof(Sgp4Elements, bstar),
        offsetof(Sgp4Elements, mean_motion_dot), offsetof(Sgp4Elements, mean_motion_ddot),
        offsetof(Sgp4Elements, eccentricity), offsetof(Sgp4Elements, arg_of_pericenter),
        offsetof(Sgp4Elements, inclination), offsetof(Sgp4Elements, mean_anomaly),
        offsetof(Sgp4Elements, mean_motion), offsetof(Sgp4Elements, ra_of_asc_node)
    };
    const double nonfinite[] = {NAN, INFINITY, -INFINITY};
    const char modes[] = {'a', 'i'};
    unsigned int m, i, j;
    for (m = 0; m < sizeof(modes); ++m) {
        const char mode = modes[m];
        int err = 99;
        Sgp4Elements e;
        Sgp4Satellite *sat;
        CHECK(invalid(NULL, mode) == 0);
        CHECK(sgp4_create(&base, mode, NULL) == NULL);
        for (i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i) {
            for (j = 0; j < sizeof(nonfinite) / sizeof(nonfinite[0]); ++j) {
                e = base;
                memcpy((char *)&e + offsets[i], &nonfinite[j], sizeof(double));
                CHECK(invalid(&e, mode) == 0);
            }
        }
        e = base; e.eccentricity = -1e-12; CHECK(invalid(&e, mode) == 0);
        e = base; e.eccentricity = 1; CHECK(invalid(&e, mode) == 0);
        e = base; e.inclination = -1e-12; CHECK(invalid(&e, mode) == 0);
        e = base; e.inclination = 3.14159265358979323846 + 1e-12; CHECK(invalid(&e, mode) == 0);
        e = base; e.mean_motion = 0; CHECK(invalid(&e, mode) == 0);
        e = base; e.mean_motion = -1e-12; CHECK(invalid(&e, mode) == 0);
        e = base; memset(e.satnum, '1', sizeof(e.satnum)); CHECK(invalid(&e, mode) == 0);
        for (i = 0; i < 5; ++i) {
            e = base; e.satnum[i] = '\0'; CHECK(invalid(&e, mode) == 0);
        }
        /* Boundary values accepted by the public contract. */
        for (i = 0; i < 2; ++i) {
            e = base; e.eccentricity = 0;
            e.inclination = i == 0 ? 0 : 3.14159265358979323846;
            memcpy(e.satnum, "A1234", 6);
            sat = sgp4_create(&e, mode, &err);
            CHECK(sat != NULL && err == 0);
            sgp4_destroy(sat);
        }
        /* Initialization can report decay even when sgp4init returns true. */
        e = base; e.mean_motion = 1.0;
        sat = sgp4_create(&e, mode, &err);
        CHECK(sat == NULL && err == 6);

        sat = sgp4_create(&base, mode, &err);
        CHECK(sat != NULL && err == 0);
        for (i = 0; i < 6; ++i) {
            Sgp4Vector p = {11, 22, 33}, v = {44, 55, 66};
            int result;
            if (i == 0) result = sgp4_predict(NULL, 0, &p, &v);
            else if (i == 1) result = sgp4_predict(sat, 0, NULL, &v);
            else if (i == 2) result = sgp4_predict(sat, 0, &p, NULL);
            else result = sgp4_predict(sat, nonfinite[i - 3], &p, &v);
            CHECK(result == -1);
            CHECK(p.x == 11 && p.y == 22 && p.z == 33);
            CHECK(v.x == 44 && v.y == 55 && v.z == 66);
        }
        {
            Sgp4Vector p, v;
            CHECK(sgp4_predict(sat, 0, &p, &v) == 0);
            CHECK(isfinite(p.x) && isfinite(p.y) && isfinite(p.z));
            CHECK(isfinite(v.x) && isfinite(v.y) && isfinite(v.z));
        }
        sgp4_destroy(sat);
    }
    sgp4_destroy(NULL);
    puts("C API validation and lifecycle tests passed");
    return 0;
}
