#include "sgp4_bridge.h"
#include "SGP4.h"
#include "test_support.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

static void check_vector(const Sgp4Vector &actual, const double expected[3])
{
    CHECK(std::fabs(actual.x - expected[0]) < 1e-12);
    CHECK(std::fabs(actual.y - expected[1]) < 1e-12);
    CHECK(std::fabs(actual.z - expected[2]) < 1e-12);
}

static void check_equivalence(const Sgp4Elements &elements)
{
    const char modes[] = {'a', 'i'};
    elsetrec direct[2]{};
    Sgp4Satellite *wrapped[2];
    int err;
    for (int i = 0; i < 2; ++i) {
        CHECK(SGP4Funcs::sgp4init(wgs72, modes[i], elements.satnum,
            elements.epoch, elements.bstar, elements.mean_motion_dot,
            elements.mean_motion_ddot, elements.eccentricity,
            elements.arg_of_pericenter, elements.inclination,
            elements.mean_anomaly, elements.mean_motion,
            elements.ra_of_asc_node, direct[i]));
        wrapped[i] = sgp4_create(&elements, modes[i], &err);
        CHECK(wrapped[i] != nullptr && err == 0);
    }

    /* Interleave satellites with different modes and revisit earlier times. */
    const double times[] = {0.0, 1440.0, -1440.0, 360.0, 360.0, 0.0};
    for (double time : times) {
        for (int i = 0; i < 2; ++i) {
            double r[3], v[3];
            CHECK(SGP4Funcs::sgp4(direct[i], time, r, v));
            Sgp4Vector position, velocity;
            CHECK(sgp4_predict(wrapped[i], time, &position, &velocity) == 0);
            check_vector(position, r);
            check_vector(velocity, v);
        }
    }
    for (auto sat : wrapped) {
        sgp4_destroy(sat);
    }
}

void test_modes()
{
    /* Synthetic valid elements: near-Earth and low-inclination deep-space. */
    Sgp4Elements elements = {"12345", 20000.0, 1e-5, 0.0, 0.0,
        0.01, 0.5, 0.9, 0.2, 0.067, 1.0};
    CHECK(test_modes_c(&elements) == 0);
    check_equivalence(elements);
    elements.inclination = 0.05;
    elements.mean_motion = 0.0044;
    elements.ra_of_asc_node = -1.0;
    check_equivalence(elements);

    const char invalid_modes[] = {'\0', 'x', 'A', 'I'};
    for (char mode : invalid_modes) {
        int err = 99;
        CHECK(sgp4_create(&elements, mode, &err) == nullptr);
        CHECK(err == -1);
    }
    CHECK(sgp4_create(&elements, 'i', nullptr) == nullptr);
    const char valid_modes[] = {'a', 'i'};
    for (char mode : valid_modes) {
        int err = 99;
        CHECK(sgp4_create(nullptr, mode, &err) == nullptr);
        CHECK(err == -1);
    }
    std::puts("Operation mode tests passed");
}
