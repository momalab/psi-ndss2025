#include "io.h"

#include <iostream>
#include <cstdint>
#include <fstream>
#include <string>
#include <sstream>
#include <tuple>
#include <vector>
#include <unordered_map>
#include "math.h"

using namespace std;

namespace io
{

ComputeParameters::ComputeParameters(const unordered_map<string, string> & params)
try
{
    ip = params.at("ip");
    port_setup = stoi(params.at("port_setup"));
    port_intersect = stoi(params.at("port_intersect"));
    rcvbuf_size = stoi(params.at("rcvbuf_size"));
    sndbuf_size = stoi(params.at("sndbuf_size"));
    num_threads = stoull(params.at("num_threads"));
}
catch (const exception & e) { throw "Error when parsing computing parameters"; }

EncryptionParameters::EncryptionParameters(const unordered_map<string, string> & params, const string & key)
try
{
    filename_gk = params.at("path") + params.at(key + "_keys") + ".gk.key";
    filename_rk = params.at("path") + params.at(key + "_keys") + ".rk.key";
    filename_sk = params.at("path") + params.at(key + "_keys") + ".sk.key";
    n = 1 << stoull(params.at(key + "_logn"));
    auto logq = split(params.at(key + "_logqi"), ',');
    for (auto & l : logq) logqi.push_back(stoi(l));
    auto t = split(params.at("ti"), ',');
    for (auto & ti : t) this->ti.push_back(stoull(ti));
    eta = stoull(params.at(key + "_eta"));
}
catch (const exception & e) { throw "Error when parsing encryption parameters"; }

SetParameters::SetParameters(const unordered_map<string, string> & params)
try
{
    auto filenames = split(params.at("set"), ',');
    for (auto & f : filenames) this->filenames.push_back(params.at("path") + f);
    bitsize = stoull(params.at("bit_size"));
}
catch (const exception & e) { throw "Error when parsing set parameters"; }

TableParameters::TableParameters(const unordered_map<string, string> & params)
try
{
    filename = params.at("path") + params.at("table");
    num_hashes = stoull(params.at("num_hashes"));
    num_tables = split(params.at("ti"), ',').size();
    table_size = 1LL << (stoull(params.at("log_table_size")) - (num_tables-1LL));
    max_data = math::shiftLeft(1ULL, stoull(params.at("bit_size"))) - 1ULL;
    max_depth = stoull(params.at("max_depth"));
}
catch (const exception & e) { throw "Error when parsing hash table parameters"; }

ostream & operator<<(ostream & os, const ComputeParameters & params)
{
    os << "IP: " << params.ip << endl;
    os << "Port (setup): " << params.port_setup << endl;
    os << "Port (intersect): " << params.port_intersect << endl;
    os << "Receive buffer size: " << params.rcvbuf_size << endl;
    os << "Send buffer size: " << params.sndbuf_size << endl;
    os << "Number of threads: " << params.num_threads << endl;
    return os;
}

ostream & operator<<(ostream & os, const EncryptionParameters & params)
{
    os << "Secret key filename: " << params.filename_sk << endl;
    os << "Relinearization keys filename: " << params.filename_rk << endl;
    os << "Galois keys filename: " << params.filename_gk << endl;
    os << "n = " << params.n << endl;
    auto q = math::sum(params.logqi);
    os << "|q| = " << q << " bits (";
    if (params.logqi.size() > 0) os << params.logqi[0];
    for (size_t i = 1; i < params.logqi.size(); i++) os << " + " << params.logqi[i];
    os << ")" << endl;
    auto t = math::sum(params.ti);
    os << "t = " << t << " (";
    if (params.ti.size() > 0) os << params.ti[0];
    for (size_t i = 1; i < params.ti.size(); i++) os << " + " << params.ti[i];
    os << ")" << endl;
    return os;
}

ostream & operator<<(ostream & os, const SetParameters & params)
{
    os << "Set filenames:" << endl;
    for (auto & f : params.filenames) os << f << endl;
    os << "Bit size: " << params.bitsize << endl;
    return os;
}

ostream & operator<<(ostream & os, const TableParameters & params)
{
    os << "Table filename: " << params.filename << endl;
    os << "Number of hashes: " << params.num_hashes << endl;
    os << "Table size: " << params.table_size << endl;
    os << "Max data: " << params.max_data << endl;
    os << "Max depth: " << params.max_depth << endl;
    os << "Number of tables: " << params.num_tables << endl;
    return os;
}

tuple<bool, ComputeParameters, EncryptionParameters, EncryptionParameters, SetParameters, TableParameters>
processInput(int argc, char * argv[])
{
    if (argc != 2) return {false, ComputeParameters(), EncryptionParameters(), EncryptionParameters(), SetParameters(), TableParameters()};
    string filename = argv[1];
    ifstream file(filename);
    if (!file.is_open()) throw "Cannot open file " + filename;

    unordered_map<string, string> params;
    string line;
    while (getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        size_t pos = line.find('=');
        if (pos == string::npos) throw "Invalid line: " + line;
        string key = trim(line.substr(0, pos));
        string value = trim(line.substr(pos + 1));
        params[key] = value;
    }
    
    if (params.find("path") == params.end()) throw "Path not specified";
    if (params["path"].back() != '/') params["path"].push_back('/');
    
    ComputeParameters compute_params(params);
    SetParameters set_params(params);
    EncryptionParameters sender_params(params, "sender");
    EncryptionParameters receiver_params(params, "receiver");
    TableParameters table_params(params);

    return {true, compute_params, sender_params, receiver_params, set_params, table_params};
}

// Save a vector of uint64_t to a file
void save(const string & filename, const vector<uint64_t> & v)
{
    ofstream file(filename);
    if (!file.is_open()) throw "Cannot open file " + filename;
    for (auto & e : v) file << e << endl;
}

// Function to split a string by a delimiter
vector<string> split(const string & s, char delimiter)
{
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter))
        tokens.push_back(trim(token));
    return tokens;
}

// Function to trim whitespace from both ends of a string
string trim(const string & str)
{
    const char* whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == string::npos) return "";
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

void usageMessage(char * argv[])
{
    cerr << "Usage: " << argv[0] << " <parameter_file>" << endl;
}

} // io