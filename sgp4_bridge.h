/*
 * sgp4_bridge.h
 * C-compatible interface to Vallado's SGP4 C++ library.
 */
#ifndef SGP4_BRIDGE_H
#define SGP4_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Sgp4Satellite Sgp4Satellite;

typedef struct {
    double x;
    double y;
    double z;
} Sgp4Vector;

/* Input values must already be converted from OMM into SGP4 units.
 * satnum:             Five-character, NUL-terminated catalog ID (Alpha-5 if needed).
 * epoch:              Julian date minus 2433281.5 (days since 1949-12-31 UTC).
 * bstar:              SGP4 drag term (inverse Earth radii).
 * mean_motion_dot:    TLE-convention first derivative, radians/minute^2.
 * mean_motion_ddot:   TLE-convention second derivative, radians/minute^3.
 * eccentricity:       Dimensionless.
 * arg_of_pericenter:  Radians.
 * inclination:        Radians.
 * mean_anomaly:       Radians.
 * mean_motion:        Radians/minute.
 * ra_of_asc_node:     Radians.
 */
typedef struct {
    char satnum[6];
    double epoch;
    double bstar;
    double mean_motion_dot;
    double mean_motion_ddot;
    double eccentricity;
    double arg_of_pericenter;
    double inclination;
    double mean_anomaly;
    double mean_motion;
    double ra_of_asc_node;
} Sgp4Elements;

/* Return codes for *err in sgp4_create() and for sgp4_predict():
 *   0  Success
 *  -1  Invalid arguments
 *  -2  Allocation failure (create only)
 *  -3  SGP4 failure without a reported error code
 *  1-6 Vallado SGP4 errors (not all can occur during initialization):
 *   1  Invalid mean elements
 *   2  Invalid mean motion
 *   3  Invalid perturbed eccentricity
 *   4  Negative semi-latus rectum
 *   5  Sub-orbital epoch elements
 *   6  Satellite decayed
 *
 * sgp4_create returns NULL on failure and writes the error to *err.
 * err must be non-NULL. On success, the caller owns the returned handle
 * and must eventually call sgp4_destroy().
 */
Sgp4Satellite *sgp4_create(const Sgp4Elements *elements, int *err);

/* Propagate by minutes relative to the element epoch (negative is allowed).
 * On success, position is TEME kilometres and velocity is TEME km/s.
 * The handle is modified by propagation; do not share it across threads
 * without synchronization. Outputs are unchanged on failure.
 */
int sgp4_predict(Sgp4Satellite *sat, double minutes_since_epoch,
                 Sgp4Vector *position, Sgp4Vector *velocity);

/* Safe to call with NULL. */
void sgp4_destroy(Sgp4Satellite *sat);

#ifdef __cplusplus
}
#endif
#endif
