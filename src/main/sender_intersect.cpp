#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include "bfv.h"
#include "crt.h"
#include "crypto_io.h"
#include "crypto_network.h"
#include "io.h"
#include "psi.h"
#include "seal/seal.h"
#include "socket.h"

using namespace fhe;
using namespace math;
using namespace network;
using namespace io;
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

    cout << "Sender's Set Intersection" << endl << endl;

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
  
    // Load Sender's keys
    cout << "Loading Sender's keys..." << flush;
    SEALContext* sender_context_ptr;
    SecretKey* sender_secret_key_ptr;
    do
    {
        start = high_resolution_clock::now();
        sender_context_ptr = instantiateEncryptionScheme(sender.n, sender.logqi, sender.ti);
        sender_secret_key_ptr = loadSecretKey(sender.filename_sk, sender_context_ptr);
        end = high_resolution_clock::now();
    } while (!validKeys(sender_context_ptr));
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_io_all += time_span;

    // Generate Sender's encoder and decryptor
    cout << "Generating Sender's encoder and decryptor..." << flush;
    start = high_resolution_clock::now();
    auto sender_encoder_ptr = new BatchEncoder(*sender_context_ptr);
    auto sender_decryptor_ptr = new Decryptor(*sender_context_ptr, *sender_secret_key_ptr);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_compute_all += time_span;
    
    // Load Receiver's evaluation keys
    cout << "Loading Receiver's evaluation keys..." << flush;
    SEALContext* receiver_context_ptr;
    RelinKeys* receiver_relinkeys_ptr;
    GaloisKeys* receiver_galoiskeys_ptr;
    do
    {
        start = high_resolution_clock::now();
        receiver_context_ptr = instantiateEncryptionScheme(receiver.n, receiver.logqi, receiver.ti);
        receiver_relinkeys_ptr = loadRelinKeys(receiver.filename_rk, receiver_context_ptr);
        receiver_galoiskeys_ptr = loadGaloisKeys(receiver.filename_gk, receiver_context_ptr);
        end = high_resolution_clock::now();
    } while (!validKeys(receiver_context_ptr));
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_io_all += time_span;

    // Generate Receiver's encoder and evaluator
    cout << "Generating Receiver's encoder and evaluator..." << flush;
    start = high_resolution_clock::now();
    auto receiver_encoder_ptr = new BatchEncoder(*receiver_context_ptr);
    auto receiver_evaluator_ptr = new Evaluator(*receiver_context_ptr);
    end = high_resolution_clock::now();
    time_span = duration_cast<TimeUnit>(end - start).count();
    cout << "done (" << time_span << " " << time_unit << ")" << endl;
    time_compute_all += time_span;

    // Wait for Receiver to connect
    cout << "Waiting for Receiver to connect..." << flush;
    Socket socket(compute.port_intersect, compute.rcvbuf_size, compute.sndbuf_size);
    socket.open();
    cout << "done." << endl;

    // Function to show times
    auto showTimes = [](string set, string t, uint64_t time, string unit) -> void
    { cout << set << " time (" << t << "): " << time << " " << unit << endl; };

    // Show pre-computation times
    cout << endl;
    showTimes("Pre-computation", "compute", time_compute_all, time_unit);
    showTimes("Pre-computation", "network", time_network_all, time_unit);
    showTimes("Pre-computation", "I/O", time_io_all, time_unit);

    cout << endl << "Recurrent costs" << endl << endl;

    // Receive the number of Receiver's sets so the program can terminate
    cout << "Receiving the number of Receiver's sets..." << flush;
    uint64_t num_sets;
    socket.receive() >> num_sets;
    cout << "done." << endl;

    // For each Receiver's set
    for (uint64_t set_number=1; set_number<=num_sets; set_number++)
    {
        cout << endl;
        
        // Reset intersection times
        time_compute_one = time_network_one = time_io_one = 0;

        // Receive intermediate results from Receiver
        cout << "Receiving intermediate results from Receiver..." << flush;
        start = high_resolution_clock::now();
        auto results = receiveCiphertexts(socket, receiver_context_ptr);
        auto randoms = receiveCiphertexts(socket, receiver_context_ptr);
        end = high_resolution_clock::now();
        time_span = duration_cast<TimeUnit>(end - start).count();
        cout << "done (" << time_span << " " << time_unit << ")" << endl;
        time_network_one += time_span;

        // Decrypt intermediate results
        cout << "Decrypting intermediate results..." << flush;
        start = high_resolution_clock::now();
        vector<vector<Ciphertext>> finals;
        recrypt
        (
            finals, results, randoms, crt, receiver.eta, sender_encoder_ptr, sender_decryptor_ptr,
            receiver_context_ptr, receiver_encoder_ptr, receiver_evaluator_ptr, receiver_relinkeys_ptr,
            receiver_galoiskeys_ptr, compute.num_threads
        );
        end = high_resolution_clock::now();
        time_span = duration_cast<TimeUnit>(end - start).count();
        cout << "done (" << time_span << " " << time_unit << ")" << endl;
        time_compute_one += time_span;

        // Send final results to Receiver
        cout << "Sending final results to Receiver..." << flush;
        start = high_resolution_clock::now();
        sendCiphertexts(socket, finals);
        end = high_resolution_clock::now();
        time_span = duration_cast<TimeUnit>(end - start).count();
        cout << "done (" << time_span << " " << time_unit << ")" << endl;
        time_network_one += time_span;

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