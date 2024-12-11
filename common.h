#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <random>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdexcept>
#include <vector>

// Constants for PMEM configuration
const size_t PAGE_SIZE = 4096;          // Page size in bytes
const size_t NUM_PAGES = 1000;         // Number of pages
const size_t PMEM_FILE_SIZE = PAGE_SIZE * NUM_PAGES; // Total PMEM file size
extern const char* PMEM_FILE_PATH;

// Write structure for generating random write data
struct Write {
    std::vector<uint8_t> data;

    Write(const std::string& str) : data(PAGE_SIZE, 0) {
        memcpy(data.data(), str.c_str(), str.size());
    }

    const uint8_t* get_page() const {
        return data.data();
    }

    size_t get_length() const {
        return data.size();
    }
};

// Function to count the number of bit flips between two data buffers
inline size_t count_bit_flips(const uint8_t* data1, const uint8_t* data2, size_t size) {
    size_t flips = 0;
    for (size_t i = 0; i < size; ++i) {
        flips += __builtin_popcount(data1[i] ^ data2[i]);
    }
    return flips;
}

// Function to initialize and map the PMEM file
inline uint8_t* init_pmem() {
    int fd = open(PMEM_FILE_PATH, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        throw std::runtime_error("Failed to open persistent memory file.");
    }

    if (ftruncate(fd, PMEM_FILE_SIZE) != 0) {
        close(fd);
        throw std::runtime_error("Failed to set persistent memory file size.");
    }

    uint8_t* pmem = (uint8_t*)mmap(nullptr, PMEM_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (pmem == MAP_FAILED) {
        throw std::runtime_error("Failed to map persistent memory.");
    }

    return pmem;
}

// Function to reset PMEM content
inline void reset_pmem(uint8_t* pmem) {
    // memset(pmem, 0, PMEM_FILE_SIZE); // Set all memory to zero
    srand(time(NULL));
    for (size_t i = 0; i < PMEM_FILE_SIZE; i++) {
        ((unsigned char *)pmem)[i] = rand() & 0xFF;
    }
}

// Function to generate a random string of specified length
inline std::string generate_random_string(size_t length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result;
    result.reserve(length);
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    for (size_t i = 0; i < length; i++) {
        result += charset[dist(rng)];
    }
    return result;
}

// Function to count the number of differing bits (Hamming distance) between two pages up to a given length
inline size_t count_hamming_distance(const uint8_t* page1, const uint8_t* page2, size_t length) {
    size_t differing_bits = 0;
    for (size_t i = 0; i < length; ++i) {
        differing_bits += __builtin_popcount(page1[i] ^ page2[i]); // Count differing bits
    }
    return differing_bits;
}

// Function to calculate the Hamming distance percentage between two pages
inline double calculate_hamming_distance_percentage(const uint8_t* page1, const uint8_t* page2, size_t length) {
    size_t total_bits = length * 8;
    size_t differing_bits = count_hamming_distance(page1, page2, length);
    return (static_cast<double>(differing_bits) / total_bits) * 100;
}

#endif // COMMON_H
