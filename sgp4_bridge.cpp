/* sgp4_bridge.cpp: C-compatible wrapper around Vallado's SGP4 C++ library. */
#include "sgp4_bridge.h"
#include "SGP4.h"

#include <cmath>
#include <cstring>
#include <new>

static const gravconsttype GRAV_MODEL = wgs72;
static const char OPS_MODE = 'i';

struct Sgp4Satellite {
    elsetrec record{};
};

extern "C" Sgp4Satellite *sgp4_create(const Sgp4Elements *elements, int *err)
{
    if (err == nullptr) {
        return nullptr;
    }
    *err = -1;

    if (elements == nullptr ||
        std::memchr(elements->satnum, '\0', sizeof(elements->satnum)) !=
            elements->satnum + 5 ||
        !std::isfinite(elements->epoch) ||
        !std::isfinite(elements->bstar) ||
        !std::isfinite(elements->mean_motion_dot) ||
        !std::isfinite(elements->mean_motion_ddot) ||
        !std::isfinite(elements->eccentricity) ||
        !std::isfinite(elements->arg_of_pericenter) ||
        !std::isfinite(elements->inclination) ||
        !std::isfinite(elements->mean_anomaly) ||
        !std::isfinite(elements->mean_motion) ||
        !std::isfinite(elements->ra_of_asc_node) ||
        elements->eccentricity < 0.0 || elements->eccentricity >= 1.0 ||
        elements->inclination < 0.0 || elements->inclination > 3.14159265358979323846 ||
        elements->mean_motion <= 0.0) {
        return nullptr;
    }

    Sgp4Satellite *sat = new (std::nothrow) Sgp4Satellite{};
    if (sat == nullptr) {
        *err = -2;
        return nullptr;
    }

    const bool success = SGP4Funcs::sgp4init(
        GRAV_MODEL, OPS_MODE, elements->satnum, elements->epoch,
        elements->bstar, elements->mean_motion_dot,
        elements->mean_motion_ddot, elements->eccentricity,
        elements->arg_of_pericenter, elements->inclination,
        elements->mean_anomaly, elements->mean_motion,
        elements->ra_of_asc_node, sat->record
    );

    if (!success) {
        *err = sat->record.error != 0 ? sat->record.error : -3;
        delete sat;
        return nullptr;
    }

    *err = 0;
    return sat;
}

extern "C" int sgp4_predict(Sgp4Satellite *sat, double minutes_since_epoch,
                             Sgp4Vector *position, Sgp4Vector *velocity)
{
    if (sat == nullptr || position == nullptr || velocity == nullptr ||
        !std::isfinite(minutes_since_epoch)) {
        return -1;
    }

    double r[3];
    double v[3];
    const bool success = SGP4Funcs::sgp4(sat->record, minutes_since_epoch, r, v);
    if (!success) {
        return sat->record.error != 0 ? sat->record.error : -3;
    }

    *position = {r[0], r[1], r[2]};
    *velocity = {v[0], v[1], v[2]};
    return 0;
}

extern "C" void sgp4_destroy(Sgp4Satellite *sat)
{
    delete sat;
}
