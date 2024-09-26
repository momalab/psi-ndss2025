#include <chrono>
#include <iostream>
#include <exception>
#include <string>
#include <tuple>
#include "bfv.h"
#include "crypto_io.h"
#include "crypto_network.h"
#include "io.h"
#include "party.h"
#include "socket.h"
#include "seal/seal.h"

using namespace fhe;
using namespace io;
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

    cout << "Receiver's Setup" << endl << endl;

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

    // Generate Receiver's keys
    cout << "Generating Receiver's keys..." << flush;
    SEALContext* receiver_context_ptr;
    SecretKey* receiver_secret_key_ptr;
    RelinKeys* receiver_relinkeys_ptr;
    GaloisKeys* receiver_galoiskeys_ptr;
    do
    {
        start = high_resolution_clock::now();
        receiver_context_ptr = instantiateEncryptionScheme(receiver.n, receiver.logqi, receiver.ti);
        tie(receiver_secret_key_ptr, receiver_relinkeys_ptr, receiver_galoiskeys_ptr) = generateKeys(receiver_context_ptr);
        end = high_resolution_clock::now();
    } while (!validKeys(receiver_context_ptr));
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_compute_off += time_span;

    // Saving Receiver's keys
    cout << "Saving Receiver's keys..." << flush;
    start = high_resolution_clock::now();
    saveSecretKey(receiver.filename_sk, receiver_secret_key_ptr);
    saveRelinKeys(receiver.filename_rk, receiver_relinkeys_ptr);
    saveGaloisKeys(receiver.filename_gk, receiver_galoiskeys_ptr);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_io_off += time_span;

    cout << endl << "Online phase" << endl << endl;

    // Connect to Sender
    cout << "Connecting to Sender..." << flush;
    start = high_resolution_clock::now();
    Socket socket(compute.port_setup, compute.rcvbuf_size, compute.sndbuf_size);
    socket.connect(compute.ip.c_str());
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_network_on += time_span;

    // Send Receiver's keys to Sender
    cout << "Sending Receiver's evaluation keys to Sender..." << flush;
    start = high_resolution_clock::now();
    sendRelinKeys(socket, receiver_relinkeys_ptr);
    sendGaloisKeys(socket, receiver_galoiskeys_ptr);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_network_on += time_span;
    
    // Receive Sender's keys from Sender
    cout << "Receiving Sender's evaluation keys from Sender..." << flush;
    start = high_resolution_clock::now();
    auto sender_context_ptr = instantiateEncryptionScheme(sender.n, sender.logqi, sender.ti);
    auto sender_relinkeys_ptr = receiveRelinKeys(socket, sender_context_ptr);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_network_on += time_span;

    // Save Sender's evaluation keys
    cout << "Saving Sender's evaluation keys..." << flush;
    start = high_resolution_clock::now();
    saveRelinKeys(sender.filename_rk, sender_relinkeys_ptr);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_io_on += time_span;
    
    // Receive Cuckoo hash table from Sender
    cout << "Receiving Cuckoo hash table from Sender..." << flush;
    start = high_resolution_clock::now();
    auto [cuckoo, encrypted_table] = receiveTable(socket, sender_context_ptr);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_network_on += time_span;

    // Save Cuckoo hash table
    cout << "Saving Cuckoo hash table..." << flush;
    start = high_resolution_clock::now();
    saveTable(table.filename, cuckoo, encrypted_table);
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