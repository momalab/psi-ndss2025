#pragma once

#include <iostream>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>

namespace io
{

struct ComputeParameters
{
    std::string ip;
    int port_setup;
    int port_intersect;
    int rcvbuf_size;
    int sndbuf_size;
    uint64_t num_threads;

    ComputeParameters() = default;
    ComputeParameters(const std::unordered_map<std::string, std::string> & params);

    friend std::ostream & operator<<(std::ostream & os, const ComputeParameters & params);
};

struct EncryptionParameters
{
    std::string filename_gk;
    std::string filename_rk;
    std::string filename_sk;
    uint64_t n;
    std::vector<int> logqi;
    std::vector<uint64_t> ti;
    uint64_t eta;

    EncryptionParameters() = default;
    EncryptionParameters(const std::unordered_map<std::string, std::string> & params, const std::string & key);

    friend std::ostream & operator<<(std::ostream & os, const EncryptionParameters & params);
};

struct SetParameters
{
    std::vector<std::string> filenames;
    uint64_t bitsize;

    SetParameters() = default;
    SetParameters(const std::unordered_map<std::string, std::string> & params);

    friend std::ostream & operator<<(std::ostream & os, const SetParameters & params);
};

struct TableParameters
{
    std::string filename;
    uint64_t num_hashes;
    uint64_t table_size;
    uint64_t max_data;
    uint64_t max_depth;
    uint64_t num_tables;

    TableParameters() = default;
    TableParameters(const std::unordered_map<std::string, std::string> & params);

    friend std::ostream & operator<<(std::ostream & os, const TableParameters & params);
};

std::tuple<bool, ComputeParameters, EncryptionParameters, EncryptionParameters, SetParameters, TableParameters> processInput(int argc, char * argv[]);
void save(const std::string & filename, const std::vector<uint64_t> & data);
std::vector<std::string> split(const std::string& s, char delimiter);
std::string trim(const std::string& str);
void usageMessage(char * argv[]);

} // io