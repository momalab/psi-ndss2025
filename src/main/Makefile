ROOT=$(abspath ../..)
3P=$(ROOT)/3p
SRC=$(ROOT)/src
CUCKOO=$(SRC)/cuckoo
DATA=$(SRC)/data
FHE=$(SRC)/fhe
IO=$(SRC)/io
MATH=$(SRC)/math
NETWORK=$(SRC)/network
PSI=$(SRC)/psi
SEAL_INC=$(3P)/seal/include/SEAL-4.1
SEAL_LIB=$(3P)/seal/lib

CC=g++
INCS=-I$(CUCKOO) -I$(FHE) -I$(MATH) -I$(IO) -I$(NETWORK) -I$(PSI) -I$(SEAL_INC)
FLAGS=-Wall -Wextra -Werror -std=c++17 -O3
CPPS=$(CUCKOO)/hash.cpp $(CUCKOO)/kuckoo.cpp\
 $(FHE)/bfv.cpp $(FHE)/packing.cpp\
 $(IO)/crypto_io.cpp $(IO)/io.cpp\
 $(MATH)/crt.cpp $(MATH)/math.cpp $(MATH)/prime.cpp $(MATH)/random.cpp\
 $(NETWORK)/crypto_network.cpp $(NETWORK)/socket.cpp\
 $(PSI)/party.cpp $(PSI)/psi.cpp
LIBS=-lgmp -lgmpxx -pthread -L$(SEAL_LIB) -lseal-4.1
DEFS=

all: info

%: %.cpp
	$(CC) $(INCS) $(FLAGS) -o $@.exe $< $(CPPS) $(LIBS) $(DEFS)

clean:
	rm -f *.log
	rm -f *.tmp
	rm -f $(DATA)/sender/*.ct
	rm -f $(DATA)/sender/*.key
	rm -f $(DATA)/sender/*.params
	rm -f $(DATA)/sender/*.size
	rm -f $(DATA)/receiver/*.ct
	rm -f $(DATA)/receiver/*.key
	rm -f $(DATA)/receiver/*.intersect
	rm -f $(DATA)/receiver/*.params
	rm -f $(DATA)/receiver/*.size

cleanall: clean
	rm -f *.exe
	rm -f $(DATA)/sender/*.set
	rm -f $(DATA)/receiver/*.set

info:
	@echo "make filename"
