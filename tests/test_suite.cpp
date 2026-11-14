#include "SGP4.h"
#include "test_support.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

static std::vector<std::string> split(const std::string &line)
{
    std::vector<std::string> fields;
    std::istringstream stream(line);
    std::string value;
    while (std::getline(stream, value, ',')) fields.push_back(value);
    return fields;
}

std::vector<ElementCase> read_elements(const std::string &path)
{
    std::ifstream file(path);
    CHECK(file.good());
    std::string line;
    CHECK(static_cast<bool>(std::getline(file, line)));
    CHECK(line == "case_id,satnum,epoch,bstar,mean_motion_dot,mean_motion_ddot,eccentricity,arg_of_pericenter,inclination,mean_anomaly,mean_motion,ra_of_asc_node");
    std::vector<ElementCase> cases;
    while (std::getline(file, line)) {
        const auto f = split(line);
        CHECK(f.size() == 12 && f[1].size() == 5);
        Sgp4Elements e{};
        std::memcpy(e.satnum, f[1].c_str(), 6);
        double *fields[] = {&e.epoch, &e.bstar, &e.mean_motion_dot,
            &e.mean_motion_ddot, &e.eccentricity, &e.arg_of_pericenter,
            &e.inclination, &e.mean_anomaly, &e.mean_motion, &e.ra_of_asc_node};
        for (int i = 0; i < 10; ++i) *fields[i] = std::stod(f[i + 2]);
        cases.push_back({f[0], e});
    }
    return cases;
}

static bool initialize(const Sgp4Elements &e, char mode, elsetrec &record)
{
    return SGP4Funcs::sgp4init(wgs72, mode, e.satnum, e.epoch, e.bstar,
        e.mean_motion_dot, e.mean_motion_ddot, e.eccentricity,
        e.arg_of_pericenter, e.inclination, e.mean_anomaly,
        e.mean_motion, e.ra_of_asc_node, record);
}

static void compare(const std::string &context, const double actual[3],
                    const double expected[3], double tolerance)
{
    for (int axis = 0; axis < 3; ++axis) {
        if (!std::isfinite(actual[axis]) || !std::isfinite(expected[axis]) ||
            std::fabs(actual[axis] - expected[axis]) > tolerance) {
            std::fprintf(stderr, "%s axis %d: actual %.17g expected %.17g tolerance %.3g\n",
                context.c_str(), axis, actual[axis], expected[axis], tolerance);
            std::exit(1);
        }
    }
}

static void dump_state(const std::string &id, char mode, double time,
                       int error, const double r[3], const double v[3])
{
    std::cout << "STATE," << id << ',' << mode << ',' << std::setprecision(17)
              << time << ',' << error;
    for (int i = 0; i < 3; ++i) std::cout << ',' << r[i];
    for (int i = 0; i < 3; ++i) std::cout << ',' << v[i];
    std::cout << '\n';
}

static void test_reference(char mode, int expected_cases, int expected_states,
                            double rtol, double vtol, bool dump)
{
    const std::string prefix = std::string("tests/fixtures/") + mode;
    const auto cases = read_elements(prefix + "_elements.csv");
    CHECK(static_cast<int>(cases.size()) == expected_cases);
    struct State { double time, r[3], v[3]; };
    std::map<std::string, std::vector<State>> states;
    std::ifstream file(prefix + "_states.csv");
    CHECK(file.good());
    std::string line;
    CHECK(static_cast<bool>(std::getline(file, line)));
    CHECK(line == "case_id,minutes,x_km,y_km,z_km,vx_km_s,vy_km_s,vz_km_s");
    int count = 0;
    while (std::getline(file, line)) {
        const auto f = split(line);
        CHECK(f.size() == 8);
        State state{};
        state.time = std::stod(f[1]);
        for (int i = 0; i < 3; ++i) {
            state.r[i] = std::stod(f[i + 2]);
            state.v[i] = std::stod(f[i + 5]);
        }
        states[f[0]].push_back(state);
        ++count;
    }
    CHECK(count == expected_states);
    int compared = 0;
    double worst_r = 0, worst_v = 0;
    for (const auto &c : cases) {
        elsetrec direct{};
        const bool success = initialize(c.elements, mode, direct);
        int error = 99;
        Sgp4Satellite *sat = sgp4_create(&c.elements, mode, &error);
        const std::string context = c.id + " (" + c.elements.satnum + ")";
        if (mode == 'a' && std::strcmp(c.elements.satnum, "33334") == 0) {
            CHECK(direct.error == 3);
            CHECK(sat == nullptr && error == direct.error);
            CHECK(states.count(c.id) == 0);
            continue;
        }
        CHECK(success && direct.error == 0 && sat != nullptr && error == 0);
        CHECK(states.count(c.id) == 1);
        const auto &samples = states.at(c.id);
        for (const auto &sample : samples) {
            double r[3], v[3];
            CHECK(SGP4Funcs::sgp4(direct, sample.time, r, v));
            const auto label = context + " t=" + std::to_string(sample.time);
            // This extreme eccentricity case has the same ~4 mm difference
            // in the separately downloaded upstream 2020 implementation.
            const bool extreme = mode == 'a' && std::strcmp(c.elements.satnum, "23333") == 0;
            compare(label + " reference position", r, sample.r, extreme ? 1e-5 : rtol);
            compare(label + " reference velocity", v, sample.v, extreme ? 3e-9 : vtol);
            for (int axis = 0; axis < 3; ++axis) {
                worst_r = std::max(worst_r, std::fabs(r[axis] - sample.r[axis]));
                worst_v = std::max(worst_v, std::fabs(v[axis] - sample.v[axis]));
            }
            Sgp4Vector p{}, velocity{};
            CHECK(sgp4_predict(sat, sample.time, &p, &velocity) == 0);
            const double br[] = {p.x, p.y, p.z};
            const double bv[] = {velocity.x, velocity.y, velocity.z};
            compare(label + " bridge position", br, r, 1e-12);
            compare(label + " bridge velocity", bv, v, 1e-12);
            if (dump) dump_state(c.id, mode, sample.time, 0, br, bv);
            ++compared;
        }
        /* Reuse the handle in reverse time order, including repeated calls. */
        std::vector<double> times = {samples.back().time, 0.0, -1440.0, 1440.0, 0.0};
        for (const auto &sample : samples) times.push_back(sample.time);
        std::reverse(times.begin(), times.end());
        for (double time : times) {
            double r[3], v[3];
            const bool ok = SGP4Funcs::sgp4(direct, time, r, v);
            const int expected_error = ok ? 0 : (direct.error ? direct.error : -3);
            Sgp4Vector p = {11, 22, 33}, velocity = {44, 55, 66};
            CHECK(sgp4_predict(sat, time, &p, &velocity) == expected_error);
            if (ok) {
                const double br[] = {p.x, p.y, p.z}, bv[] = {velocity.x, velocity.y, velocity.z};
                compare(context + " reverse position", br, r, 1e-12);
                compare(context + " reverse velocity", bv, v, 1e-12);
                // A fresh record must agree despite previous call order.
                elsetrec fresh{};
                CHECK(initialize(c.elements, mode, fresh));
                CHECK(SGP4Funcs::sgp4(fresh, time, r, v));
                compare(context + " fresh position", br, r, 1e-8);
                compare(context + " fresh velocity", bv, v, 1e-11);
            } else {
                CHECK(p.x == 11 && p.y == 22 && p.z == 33);
                CHECK(velocity.x == 44 && velocity.y == 55 && velocity.z == 66);
            }
        }
        sgp4_destroy(sat);
    }
    CHECK(compared == expected_states);
    std::printf("Mode %c: %d cases, %d reference states; max component error %.3g km / %.3g km/s\n",
                mode, expected_cases, compared, worst_r, worst_v);
}

int main(int argc, char **argv)
{
    const bool dump = argc == 2 && std::strcmp(argv[1], "--dump") == 0;
    CHECK(argc == 1 || dump);
    test_modes();
    CHECK(test_api() == 0);
    test_reference('a', 33, 666, 1e-6, 1e-9, dump);
    test_reference('i', 20, 100, 1e-7, 1e-10, dump);
    const auto cases = read_elements("tests/fixtures/a_elements.csv");
    const char modes[] = {'a', 'i'};
    for (char mode : modes) {
        CHECK(test_propagation_failure(&cases.at(11).elements, mode,
            1440, 1, "error_mean_elements", dump) == 0);
        CHECK(test_propagation_failure(&cases.at(29).elements, mode,
            25, 4, "error_semilatus_rectum", dump) == 0);
        CHECK(test_propagation_failure(&cases.at(25).elements, mode,
            60, 6, "error_decay", dump) == 0);
    }
    std::puts("Real SGP4 propagation failures and recovery passed");
    std::puts("All tests passed");
}
