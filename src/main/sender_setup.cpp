#include <chrono>
#include <iostream>
#include <exception>
#include <functional>
#include <string>
#include <tuple>
#include <vector>
#include "bfv.h"
#include "crt.h"
#include "crypto_io.h"
#include "crypto_network.h"
#include "kuckoo.h"
#include "io.h"
#include "packing.h"
#include "party.h"
#include "seal/seal.h"
#include "socket.h"

using namespace cuckoo;
using namespace fhe;
using namespace io;
using namespace math;
using namespace network;
using namespace psi;
using namespace seal;
using namespace std;
using namespace std::chrono;

using TimeUnit = milliseconds;
const string time_unit = "ms";

int main(int argc, char * argv[])
try
{
    auto [success, compute, sender, receiver, set, table] = processInput(argc, argv);
    if (!success) { usageMessage(argv); return 1; }

    cout << "Sender's Setup" << endl << endl;

    cout << "Compute parameters:" << endl << compute << endl;
    cout << "Sender parameters:" << endl << sender << endl;
    cout << "Receiver parameters:" << endl << receiver << endl;
    cout << "Set parameters:" << endl << set << endl;
    cout << "Table parameters:" << endl << table << endl;

    time_point<high_resolution_clock> start, end;
    uint64_t time_span;
    uint64_t time_compute_on , time_network_on , time_io_on ;
    uint64_t time_compute_off, time_network_off, time_io_off;
    time_compute_on = time_network_on = time_io_on = 0;
    time_compute_off = time_network_off = time_io_off = 0;

    cout << endl << "Offline phase" << endl << endl;

    // CRT parameters
    cout << "Calculating CRT parameters..." << flush;
    start = high_resolution_clock::now();
    auto crt = crtParams(sender.ti);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_compute_off += time_span;

    // Loading Sender's set
    cout << "Loading Sender's set..." << flush;
    start = high_resolution_clock::now();
    Party party(set.filenames[0]);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_io_off += time_span;

    // Generate Sender's keys
    cout << "Generating Sender's keys..." << flush;
    SEALContext* sender_context_ptr;
    SecretKey* sender_secret_key_ptr;
    RelinKeys* sender_relinkeys_ptr;
    GaloisKeys* sender_galoiskeys_ptr;
    do
    {
        start = high_resolution_clock::now();
        sender_context_ptr = instantiateEncryptionScheme(sender.n, sender.logqi, sender.ti);
        tie(sender_secret_key_ptr, sender_relinkeys_ptr, sender_galoiskeys_ptr) = generateKeys(sender_context_ptr, false);
        end = high_resolution_clock::now();
    } while (!validKeys(sender_context_ptr));
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_compute_off += time_span;

    // Saving Sender's keys
    cout << "Saving Sender's keys..." << flush;
    start = high_resolution_clock::now();
    saveSecretKey(sender.filename_sk, sender_secret_key_ptr);
    saveRelinKeys(sender.filename_rk, sender_relinkeys_ptr);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_io_off += time_span;

    // k-table Cuckoo hashing
    cout << "Generating Cuckoo hash table..." << flush;
    start = high_resolution_clock::now();
    Kuckoo cuckoo(table.num_hashes, table.table_size, table.max_data, table.max_depth, table.num_tables);
    cuckoo.insert(party.getSet());
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_compute_off += time_span;

    // Encode and Encrypt Cuckoo hash table
    cout << "Encrypting Cuckoo hash table..." << flush;
    start = high_resolution_clock::now();
    auto sender_encoder_ptr = new BatchEncoder(*sender_context_ptr);
    auto sender_encryptor_ptr = new Encryptor(*sender_context_ptr, *sender_secret_key_ptr);
    vector<Ciphertext> encrypted_table;
    packEncrypt(encrypted_table, cuckoo.getTable(), crt, sender_encoder_ptr, sender_encryptor_ptr, compute.num_threads);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_compute_off += time_span;

    // Save Cuckoo hash table
    cout << "Saving Cuckoo hash table..." << flush;
    start = high_resolution_clock::now();
    saveTable(table.filename, cuckoo, encrypted_table);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_io_off += time_span;

    cout << endl << "Online phase" << endl << endl;

    // Wait for Receiver to connect
    cout << "Waiting for Receiver to connect..." << flush;
    Socket socket(compute.port_setup, compute.rcvbuf_size, compute.sndbuf_size);
    socket.open();
    cout << "done." << endl;

    // Receive Receiver's keys
    cout << "Receiving Receiver's evaluation keys..." << flush;
    start = high_resolution_clock::now();
    auto receiver_context_ptr = instantiateEncryptionScheme(receiver.n, receiver.logqi, receiver.ti);
    auto receiver_relinkeys_ptr = receiveRelinKeys(socket, receiver_context_ptr);
    auto receiver_galoiskeys_ptr = receiveGaloisKeys(socket, receiver_context_ptr);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_network_on += time_span;

    // Send Sender's keys to Receiver
    cout << "Sending Sender's evaluation keys to Receiver..." << flush;
    start = high_resolution_clock::now();
    sendRelinKeys(socket, sender_relinkeys_ptr);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_network_on += time_span;

    // Send Cuckoo hash table to Receiver
    cout << "Sending Cuckoo hash table to Receiver..." << flush;
    start = high_resolution_clock::now();
    sendTable(socket, cuckoo, encrypted_table);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_network_on += time_span;

    // Save Receiver's evaluation keys
    cout << "Saving Receiver's evaluation keys..." << flush;
    start = high_resolution_clock::now();
    saveRelinKeys(receiver.filename_rk, receiver_relinkeys_ptr);
    saveGaloisKeys(receiver.filename_gk, receiver_galoiskeys_ptr);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_io_on += time_span;

    // Show total time
    auto showTimes = [](string t, uint64_t off, uint64_t on, string unit) -> void
    { cout << "Total time (offline + online " << t << "): " << off << " " << unit << " + " << on << " " << unit << " = " << (off + on) << " " << unit << endl; };

    cout << endl;
    showTimes("compute", time_compute_off, time_compute_on, time_unit);
    showTimes("network", time_network_off, time_network_on, time_unit);
    showTimes("I/O", time_io_off, time_io_on, time_unit);
}
catch (const exception & e) { cerr << e.what() << endl; return 1; }
catch (const char * e) { cerr << e << endl; return 1; }
catch (const string & e) { cerr << e << endl; return 1; }
catch (...) { cerr << "Unknown exception" << endl; return 1; }