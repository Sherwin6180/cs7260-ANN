#include "common.h"
#include <fstream>
#include <sstream>
#include <functional>

// Specify PMEM file path
const char* PMEM_FILE_PATH = "/mnt/pmem/testfile";

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <csv_file>" << std::endl;
        return 1;
    }

    try {
        // Initialize PMEM
        uint8_t* pmem = init_pmem();

        // Reset PMEM to ensure consistent starting state
        reset_pmem(pmem);

        // Populate PMEM with initial data from CSV
        std::ifstream init_file(argv[1]);
        if (!init_file.is_open()) {
            throw std::runtime_error("Failed to open CSV file for initialization.");
        }

        std::cout << "Populating PMEM from CSV file..." << std::endl;
        std::string line;
        while (std::getline(init_file, line)) {
            std::istringstream ss(line);
            std::string key, value;
            if (!std::getline(ss, key, ',') || !std::getline(ss, value)) {
                continue; // Skip malformed lines
            }

            // Compute page index based on hashed key
            std::hash<std::string> hasher;
            size_t hash_value = hasher(key);
            size_t page_index = hash_value % NUM_PAGES;
            uint8_t* page_addr = pmem + (page_index * PAGE_SIZE);

            // Write value to the determined page
            memcpy(page_addr, value.data(), std::min(value.size(), PAGE_SIZE));
        }
        init_file.close();

        // Open the CSV file again for write queries
        std::ifstream query_file(argv[1]);
        if (!query_file.is_open()) {
            throw std::runtime_error("Failed to open CSV file for queries.");
        }

        size_t total_bit_flips = 0;
        size_t query_count = 0;

        std::cout << "Processing write queries from CSV file (first 1000 entries)..." << std::endl;
        while (std::getline(query_file, line) && query_count < 100000) {
            std::istringstream ss(line);
            std::string key, value;
            if (!std::getline(ss, key, ',') || !std::getline(ss, value)) {
                continue; // Skip malformed lines
            }

            // Select a random page for the write operation
            size_t page_index = rand() % NUM_PAGES;
            uint8_t* page_addr = pmem + (page_index * PAGE_SIZE);

            // Perform the write operation
            Write write(value);
            total_bit_flips += count_bit_flips(page_addr, write.get_page(), PAGE_SIZE);
            memcpy(page_addr, write.get_page(), PAGE_SIZE);

            ++query_count;
        }

        query_file.close();
        std::cout << "Total bit flips (default behavior): " << total_bit_flips << std::endl;

        // Cleanup
        munmap(pmem, PMEM_FILE_SIZE);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
