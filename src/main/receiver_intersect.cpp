#include <chrono>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "bfv.h"
#include "crt.h"
#include "crypto_io.h"
#include "crypto_network.h"
#include "io.h"
#include "party.h"
#include "psi.h"
#include "seal/seal.h"
#include "socket.h"

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

    cout << "Receiver's Set Intersection" << endl << endl;

    cout << "Compute parameters:" << endl << compute << endl;
    cout << "Sender parameters:" << endl << sender << endl;
    cout << "Receiver parameters:" << endl << receiver << endl;
    cout << "Set parameters:" << endl << set << endl;
    cout << "Table parameters:" << endl << table << endl;

    time_point<high_resolution_clock> start, end;
    uint64_t time_span;
    uint64_t time_compute_one, time_network_one, time_io_one;
    uint64_t time_compute_all, time_network_all, time_io_all;
    time_compute_one = time_network_one = time_io_one = 0;
    time_compute_all = time_network_all = time_io_all = 0;

    cout << endl << "One-time costs" << endl << endl;

    // CRT parameters
    cout << "Calculating CRT parameters..." << flush;
    start = high_resolution_clock::now();
    auto crt = crtParams(sender.ti);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_compute_all += time_span;

    // Load Receiver's keys
    cout << "Loading Receiver's keys..." << flush;
    SEALContext* receiver_context_ptr;
    SecretKey* receiver_secret_key_ptr;
    do
    {
        start = high_resolution_clock::now();
        receiver_context_ptr = instantiateEncryptionScheme(receiver.n, receiver.logqi, receiver.ti);
        receiver_secret_key_ptr = loadSecretKey(receiver.filename_sk, receiver_context_ptr);
        end = high_resolution_clock::now();
    } while (!validKeys(receiver_context_ptr));
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_io_all += time_span;

    // Generate Receiver's encoder, encryptor, and decryptor
    cout << "Generating Receiver's encoder, encryptor, and decryptor..." << flush;
    start = high_resolution_clock::now();
    auto receiver_encoder_ptr = new BatchEncoder(*receiver_context_ptr);
    auto receiver_encryptor_ptr = new Encryptor(*receiver_context_ptr, *receiver_secret_key_ptr);
    auto receiver_decryptor_ptr = new Decryptor(*receiver_context_ptr, *receiver_secret_key_ptr);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_compute_all += time_span;

    // Load Sender's evaluation keys
    cout << "Loading Sender's evaluation keys..." << flush;
    SEALContext* sender_context_ptr;
    RelinKeys* sender_relinkeys_ptr;
    do
    {
        start = high_resolution_clock::now();
        sender_context_ptr = instantiateEncryptionScheme(sender.n, sender.logqi, sender.ti);
        sender_relinkeys_ptr = loadRelinKeys(sender.filename_rk, sender_context_ptr);
        end = high_resolution_clock::now();
    } while (!validKeys(sender_context_ptr));
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_io_all += time_span;

    // Generate Sender's encoder and evaluator
    cout << "Generating Sender's encoder and evaluator..." << flush;
    start = high_resolution_clock::now();
    auto [sender_encoder_ptr, sender_evaluator_ptr] = generateEvaluator(sender_context_ptr);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_compute_all += time_span;

    // Load Cuckoo hash table
    cout << "Loading Cuckoo hash table..." << flush;
    start = high_resolution_clock::now();
    auto [cuckoo, encrypted_table] = loadTable(table.filename, sender_context_ptr);
    uint64_t receiver_dummy = get<3>(cuckoo.getParameters()) + 2;
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_io_all += time_span;

    // Connect to Sender
    cout << "Connecting to Sender..." << flush;
    start = high_resolution_clock::now();
    Socket socket(compute.port_intersect, compute.rcvbuf_size, compute.sndbuf_size);
    socket.connect(compute.ip.c_str());
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_network_one += time_span;

    // Function to show times
    auto showTimes = [](string set, string t, uint64_t time, string unit) -> void
    { cout << set << " time (" << t << "): " << time << " " << unit << endl; };

    // Show pre-computation times
    cout << endl;
    showTimes("Pre-computation", "compute", time_compute_all, time_unit);
    showTimes("Pre-computation", "network", time_network_all, time_unit);
    showTimes("Pre-computation", "I/O", time_io_all, time_unit);

    cout << endl << "Recurrent costs" << endl << endl;

    // Send the number of Receiver's set to Sender so it can terminate
    cout << "Sending the number of Receiver's set to Sender..." << flush;
    const uint64_t & num_sets = set.filenames.size();
    {
        stringstream ss;
        ss << num_sets;
        socket.send(ss);
    }
    cout << "done" << endl;

    // For each Receiver's set
    for (uint64_t set_number=1; set_number<=num_sets; set_number++)
    {
        cout << endl;
        
        // Set filename
        auto & set_filename = set.filenames[set_number-1];

        // Reset intersection times
        time_compute_one = time_network_one = time_io_one = 0;

        // Load Receiver's set
        cout << "Loading Receiver's set..." << flush;
        start = high_resolution_clock::now();
        Party party(set_filename);
        end = high_resolution_clock::now();
        time_span = duration_cast<TimeUnit>(end - start).count();
        cout << "done (" << time_span << " " << time_unit << ")" << endl;
        time_io_one += time_span;

        // Compute intersection
        cout << "Computing intersection..." << flush;
        start = high_resolution_clock::now();
        vector<vector<Ciphertext>> results, randoms;
        computeIntersection
        (
            results, randoms, party, cuckoo, encrypted_table, crt, sender.eta,
            sender_context_ptr, sender_encoder_ptr, sender_evaluator_ptr, sender_relinkeys_ptr,
            receiver_encoder_ptr, receiver_encryptor_ptr, receiver_dummy, compute.num_threads
        );
        end = high_resolution_clock::now();
        time_span = duration_cast<TimeUnit>(end - start).count();
        cout << "done (" << time_span << " " << time_unit << ")" << endl;
        time_compute_one += time_span;

        // Send intermediate results to Sender
        cout << "Sending intermediate results to Sender..." << flush;
        start = high_resolution_clock::now();
        sendCiphertexts(socket, results);
        sendCiphertexts(socket, randoms);
        end = high_resolution_clock::now();
        time_span = duration_cast<TimeUnit>(end - start).count();
        cout << "done (" << time_span << " " << time_unit << ")" << endl;
        time_network_one += time_span;

        // Receive final results from Sender
        cout << "Receiving final results from Sender..." << flush;
        start = high_resolution_clock::now();
        auto finals = receiveCiphertexts(socket, receiver_context_ptr);
        end = high_resolution_clock::now();
        time_span = duration_cast<TimeUnit>(end - start).count();
        cout << "done (" << time_span << " " << time_unit << ")" << endl;
        time_network_one += time_span;

        // Decrypt results
        cout << "Decrypting intersection..." << flush;
        start = high_resolution_clock::now();
        auto intersection = decryptIntersection(finals, party, crt, receiver_encoder_ptr, receiver_decryptor_ptr, compute.num_threads);
        end = high_resolution_clock::now();
        time_span = duration_cast<TimeUnit>(end - start).count();
        cout << "done (" << time_span << " " << time_unit << ")" << endl;
        time_compute_one += time_span;

        // Save intersection
        cout << "Saving intersection of size " << intersection.size() << "..." << flush;
        start = high_resolution_clock::now();
        save(set_filename + ".intersect", intersection);
        end = high_resolution_clock::now();
        time_span = duration_cast<TimeUnit>(end - start).count();
        cout << "done (" << time_span << " " << time_unit << ")" << endl;
        time_io_one += time_span;

        // Show intersection times
        cout << endl;
        auto set_name = "Set #" + to_string(set_number);
        showTimes(set_name, "compute", time_compute_one, time_unit);
        showTimes(set_name, "network", time_network_one, time_unit);
        showTimes(set_name, "I/O", time_io_one, time_unit);

        // Add times to total
        time_compute_all += time_compute_one;
        time_network_all += time_network_one;
        time_io_all += time_io_one;
    }

    // Show total times
    cout << endl;
    showTimes("Total", "compute", time_compute_all, time_unit);
    showTimes("Total", "network", time_network_all, time_unit);
    showTimes("Total", "I/O", time_io_all, time_unit);
}
catch (const exception & e) { cerr << e.what() << endl; return 1; }
catch (const char * e) { cerr << e << endl; return 1; }
catch (const string & e) { cerr << e << endl; return 1; }
catch (...) { cerr << "Unknown exception" << endl; return 1; }