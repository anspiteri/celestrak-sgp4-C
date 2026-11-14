#include "sgp4_bridge.h"
#include <stddef.h>

/* Compiled as C, then called by the C++ numerical tests. */
int test_modes_c(const Sgp4Elements *elements)
{
    const char modes[] = {'a', 'i'};
    unsigned int i;
    int err;
    for (i = 0; i < sizeof(modes); ++i) {
        Sgp4Satellite *sat = sgp4_create(elements, modes[i], &err);
        Sgp4Vector position, velocity;
        if (sat == NULL || err != 0) {
            sgp4_destroy(sat);
            return 1;
        }
        if (sgp4_predict(sat, 0.0, &position, &velocity) != 0) {
            sgp4_destroy(sat);
            return 1;
        }
        sgp4_destroy(sat);
    }
    return 0;
}
