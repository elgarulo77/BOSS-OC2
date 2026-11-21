// Uso: core_test in.f64 out.f64 knobD knobO1 knobO2 [preScale attScale relScale]
#include "../juce/Source/OctaveCore.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
int main (int argc, char** argv)
{
    if (argc < 6) return 1;
    FILE* f = fopen (argv[1], "rb"); fseek (f, 0, SEEK_END); long n = ftell (f) / 8; fseek (f, 0, SEEK_SET);
    std::vector<double> x (n); if (fread (x.data(), 8, n, f) != (size_t) n) return 2; fclose (f);
    oct::OctaveCore c; c.prepare (44100.0, argc > 6 ? atof (argv[6]) : 1.0, argc > 7 ? atof (argv[7]) : 1.0, argc > 8 ? atof (argv[8]) : 1.0);
    for (long i = 0; i < n; ++i) x[i] = c.process (x[i], atof (argv[3]), atof (argv[4]), atof (argv[5]));
    f = fopen (argv[2], "wb"); fwrite (x.data(), 8, n, f); fclose (f);
}
