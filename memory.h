#ifndef MEMORY_H
#define MEMORY_H

/* ****************************************************************
   RISC-V Instruction Set Simulator
   Computer Architecture, Semester 1, 2024

   Class for memory

**************************************************************** */

#include <array>
#include <string>
#include <unordered_map>
#include <cstdint>

using namespace std;

class memory {

 private:
     unordered_map<uint64_t, array<uint64_t, 256>> store;
     static constexpr uint64_t address_index(uint64_t address) {
         return (address >> 3) & 0xFF;
     }
     static constexpr uint64_t address_key(uint64_t address) {
         return (address >> 3) & (~0xFF);
     }

  // hints:
  //   // Store implemented as an unordered_map of vectors, each containing 4Kbytes (512 doublewords) of data.
  //   unordered_map< uint64_t, vector<uint64_t> > store;  // Initially empty
  //
  //   // Check if a page of store is allocated, and allocate if not
  //   void validate (uint64_t address);
  //
  // elaboration:
  //   Using an array instead of a vector means that it is auto-initialised. So if the memory hasn't
  //   been allocated prior, when we index into the hashmap, the array will be created for us. So
  //   just by using regular indexing, so long as we are indexing a valid memory address, we don't
  //   need to deal with validation

 public:

  // Constructor
  memory(bool verbose);
  	 
  // Read a doubleword of data from a doubleword-aligned address.
  // If the address is not a multiple of 8, it is rounded down to a multiple of 8.
  uint64_t read_doubleword (uint64_t address);

  // Write a doubleword of data to a doubleword-aligned address.
  // If the address is not a multiple of 8, it is rounded down to a multiple of 8.
  // The mask contains 1s for bytes to be updated and 0s for bytes that are to be unchanged.
  void write_doubleword (uint64_t address, uint64_t data, uint64_t mask);

  // Load a hex image file and provide the start address for execution from the file in start_address.
  // Return true if the file was read without error, or false otherwise.
  bool load_file(string file_name, uint64_t &start_address);

};

#endif
