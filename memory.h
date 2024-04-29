#ifndef MEMORY_H
#define MEMORY_H

/* ****************************************************************
   RISC-V Instruction Set Simulator
   Computer Architecture, Semester 1, 2024

   Class for memory

**************************************************************** */

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

class memory {

private:
  std::unordered_map<uint64_t, std::array<uint64_t, 256>> store;
  static constexpr uint64_t address_index(uint64_t address) {
    return (address >> 3) & 0xFF;
  }
  static constexpr uint64_t address_key(uint64_t address) {
    return (address >> 3) & (~0xFF);
  }
  void validate_address(uint64_t address);

  const bool verbose;

  // hints:
  //   // Store implemented as an unordered_map of vectors, each containing
  //   4Kbytes (512 doublewords) of data. unordered_map< uint64_t,
  //   vector<uint64_t> > store;  // Initially empty
  //
  //   // Check if a page of store is allocated, and allocate if not
  //   void validate (uint64_t address);

public:
  // Constructor
  memory(bool verbose);

  // Read a doubleword of data from a doubleword-aligned address.
  // If the address is not a multiple of 8, it is rounded down to a multiple
  // of 8.
  uint64_t read_doubleword(uint64_t address);

  // Write a doubleword of data to a doubleword-aligned address.
  // If the address is not a multiple of 8, it is rounded down to a multiple
  // of 8. The mask contains 1s for bytes to be updated and 0s for bytes that
  // are to be unchanged.
  void write_doubleword(uint64_t address, uint64_t data, uint64_t mask);

  // Load a hex image file and provide the start address for execution from the
  // file in start_address. Return true if the file was read without error, or
  // false otherwise.
  bool load_file(std::string file_name, uint64_t &start_address);
};

#endif
