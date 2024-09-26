#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "party.h"

using namespace psi;;
using namespace std;

int main(int argc, char * argv[])
try
{
    if (argc <= 3)
    {
        cerr << "Usage: " << argv[0] << " <set_size> <bit_size> <target_file> [source_file] [source_probability]" << endl;
        cerr << "  set_size: number of elements in the set" << endl;
        cerr << "  bit_size: number of bits in each element" << endl;
        cerr << "  target_file: file to save the set" << endl;
        cerr << "  source_file: file to be sample" << endl;
        cerr << "  source_probability: probability of sampling from source_file" << endl;
        return 1;
    }
    uint64_t set_size = stoull(argv[1]);
    uint64_t bit_size = stoull(argv[2]);
    string target_file = argv[3];

    cout << "Generating set with " << set_size << " elements of " << bit_size << " bits" << endl;

    Party party;
    if (argc > 4)
    {
        // read source set from file
        string source_file = argv[4];
        double probability = argc > 5 ? stod(argv[5]) : 1.0;
        Party source_party(source_file);
        // create new set
        bit_size = max(bit_size, source_party.getBitSize());
        cout << "Sourcing from " << source_file << " with probability " << probability << " with " << source_party.getSet().size() << " elements of " << source_party.getBitSize() << " bits" << endl;
        party = Party(set_size, bit_size, source_party.getSet(), probability);
    }
    else party = Party(set_size, bit_size);
    party.save(target_file);
}
catch (const exception & e) { cerr << e.what() << endl; return 1; }
catch (const char * e) { cerr << e << endl; return 1; }
catch (const string & e) { cerr << e << endl; return 1; }
catch (...) { cerr << "Unknown exception" << endl; return 1; }