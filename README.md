# Recurrent Private Set Intersection for Unbalanced Databases with Cuckoo Hashing and Leveled FHE

This project implements the Private Set Intersection (PSI) protocol[$^1$](#cite-us) tailored for unbalanced sets in the recurrent setting.
Two versions of the protocol are available:

1. **Standalone**: A single program that executes the entire protocol, suitable for reproducing paper results.
2. **Deployment**: Comprising four distinct programs for Receiver/Sender setup and Receiver/Sender set intersection.

## Table of Contents

- [Installation](#installation)
  - [Requirements](#requirements)
  - [Setup](#setup)
- [Standalone](#standalone)
- [Deployment](#deployment)
- [License](#license)
- [Cite us](#cite-us)

## Installation

### Requirements

- Linux
- Git
- GNU C++17 compiler
- Python 3

### Setup

1. Clone the repository:
   ```bash
   git clone https://github.com/momalab/psi.git
   ```

2. Install Microsoft SEAL library version 4.1:
   ```bash
   cd 3p
   bash install_seal.sh
   ```
   This script automatically clones SEAL v4.1 into `3p/clone`, compiles and tests it, and installs the library in `3p/seal`. For SEAL dependencies, refer to the [Microsoft SEAL repository](https://github.com/microsoft/SEAL).

## Standalone

The Standalone implementation allows for straightforward evaluation. To execute it:

1. Navigate to the `src/main` directory:
   ```bash
   cd src/main
   ```

2. Compile the target program `protocol.cpp`:
   ```bash
   make protocol
   ```

   Run the compiled program without arguments to see the list of parameters:
   ```bash
   ./protocol.exe
   ```
   ```
   Usage: ./protocol.exe <mode> <log2|X|> <|Y|> <|m|> <threads>
   mode: 0 (Fast Setup), 1 (Fast Intersection)
   log2|X|: log2 of the size of the Sender's set (default: 20)
   |Y|: size of the Receiver's set (default: 4)
   |m|: number of recurrencies (default: 1)
   threads: number of threads (default: 4)
   ```
   The paper proposes two configurations: *Fast Setup* and *Fast Intersection*. For each, evaluate combinations of $\log_2|X| = \{16, 20, 24\}$, $|Y| = \{4, 16, 64\}$, and $m = \{1, 4, 16, 64\}$ using 4 threads. For example, to run with $\log_2|X| = 20$, $|Y| = 4$, and $m = 16$ in Fast Intersection mode:
   ```bash
   ./protocol.exe 1 20 4 16
   python3 summary.py 16
   ```
   The first command generates synthetic sets based on input parameters and executes the protocol, while the second summarizes the results.

3. To clean temporary results, run:
   ```bash
   make clean
   ```

   To remove temporary, generated sets, and binary files, run:
   ```bash
   make cleanall
   ```

## Deployment

The Deployment implementation is designed for practical use. Follow these steps:

1. Navigate to the `src/main` directory:
   ```bash
   cd src/main
   ```

2. Generate synthetic sets:
   ```bash
   make generate_set
   ```

   Run the set generator without arguments to see the parameters it accepts:
   ```bash
   ./generate_set.exe
   ```
   ```
   Usage: ./generate_set.exe <set_size> <bit_size> <target_file> [source_file] [source_probability]
   set_size: number of elements in the set
   bit_size: number of bits in each element
   target_file: file to save the set
   source_file: file to sample from
   source_probability: probability of sampling from source_file
   ```

   For example, to create a set $X$ with 912,261 elements (an 87% load for a $2^{20}$ Cuckoo hash table) and save it to `../data/sender/X_20.set`:
   ```
   ./generate_set.exe 912261 32 ../data/sender/X_20.set
   ```

   To create four sets $Y$ of size $|Y| = 4$ with a 50% chance of including elements from set $X$:
   ```bash
   for i in {1..4}; do
       ./generate_set.exe 4 32 ../data/receiver/Y_4_$i.set ../data/sender/X_20.set 0.5
   done
   ```

3. Compile each program for the protocol:
   ```bash
   make sender_setup
   make receiver_setup
   make sender_intersect
   make receiver_intersect
   ```

4. Each program requires a configuration file as input. We provide two sets of configuration files:
   - **Fast Setup**: `fs_receiver.params` and `fs_sender.params`
   - **Fast Intersection**: `fi_receiver.params` and `fi_sender.params`

   You can modify or create new parameter files as needed.

### Protocol Setup

This part runs the one-time-cost part of the protocol. Use two terminal windows:

**Terminal 1 (Sender)**:
```
./sender_setup.exe fs_sender.params
```

**Terminal 2 (Receiver)**:
```
./receiver_setup.exe fs_receiver.params
```

### Protocol Intersection

This part executes the recurrent part of the protocol.

**Terminal 1 (Sender)**:
```
./sender_intersect.exe fs_sender.params
```

**Terminal 2 (Receiver)**:
```
./receiver_intersect.exe fs_receiver.params
```

After execution, you will find `.intersect` files in the Receiver's directory (`../data/receiver`), one for each set $Y$. For example, the intersection of set $X$ (`X_20.set`) and $Y_1$ (`Y_4_1.set`) will be stored in `Y_4_1.set.intersect`.

## License

This project is licensed under the [GNU General Public License v3](LICENSE).

## Cite us

The *Recurrent Private Set Intersection for Unbalanced Databases with Cuckoo Hashing and Leveled FHE* is accepted for publication at NDSS'25.