// pq_behavior.cpp
#include "common.h"
#include "pq_algorithm.cpp"
#include <fstream>
#include <sstream>
#include <functional>
#include <chrono>
#include <iostream>
#include <cstring>

// Specify PMEM file path
const char* PMEM_FILE_PATH = "/mnt/pmem/testfile";

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <csv_file>" << std::endl;
        return 1;
    }

    try {
        // Start measuring total execution time
        auto start_time = std::chrono::high_resolution_clock::now();

        // Initialize PMEM
        uint8_t* pmem = init_pmem();

        // Reset PMEM to ensure consistent starting state
        reset_pmem(pmem);

        // Train Product Quantizer using PMEM's current state
        std::cout << "Training PQ algorithm on PMEM content..." << std::endl;
        ProductQuantizer pq;
        pq.train(pmem, 1000);

        // Record time after training
        auto after_training_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> training_duration = after_training_time - start_time;
        std::cout << "Time taken for training: " << training_duration.count() << " seconds" << std::endl;

        // Open the CSV file for testing writes
        std::ifstream test_file(argv[1]);
        if (!test_file.is_open()) {
            throw std::runtime_error("Failed to open CSV file for testing.");
        }

        size_t total_bit_flips = 0;
        double total_hamming_distance_percentage = 0.0;
        size_t write_count = 0;

        // Read each key-value pair and perform PQ-based writes
        std::cout << "Processing write queries from CSV file..." << std::endl;
        std::string line;
        while (std::getline(test_file, line)) {
            std::istringstream ss(line);
            std::string key, value;
            if (!std::getline(ss, key, ',') || !std::getline(ss, value)) {
                continue; // Skip malformed lines
            }

            // Hash the key for write query
            std::hash<std::string> hasher;
            size_t hash_value = hasher(key);

            // Generate write object
            Write write(value);

            // Find the nearest page using PQ algorithm
            size_t page_index = pq.find_nearest_page(write.get_page());
            uint8_t* page_addr = pmem + (page_index * PAGE_SIZE);

            // Get the length of the query
            size_t query_length = write.get_length();

            // Calculate Hamming distance percentage before writing
            double hamming_distance_percentage = calculate_hamming_distance_percentage(page_addr, write.get_page(), query_length);
            total_hamming_distance_percentage += hamming_distance_percentage;
            write_count++;

            // Calculate bit flips and write to PMEM
            total_bit_flips += count_bit_flips(page_addr, write.get_page(), query_length);
            memcpy(page_addr, write.get_page(), query_length);
        }

        test_file.close();

        // Output total bit flips and average Hamming distance percentage
        std::cout << "Total bit flips (PQ behavior): " << total_bit_flips << std::endl;
        if (write_count > 0) {
            double average_hamming_distance_percentage = total_hamming_distance_percentage / write_count;
            std::cout << "Average Hamming distance percentage: " << average_hamming_distance_percentage << "%" << std::endl;
        } else {
            std::cout << "No valid write queries processed." << std::endl;
        }

        // Record time after queries
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> query_duration = end_time - after_training_time;
        std::chrono::duration<double> total_duration = end_time - start_time;

        std::cout << "Time taken for processing queries: " << query_duration.count() << " seconds" << std::endl;
        std::cout << "Total execution time: " << total_duration.count() << " seconds" << std::endl;

        // Report average timings from ProductQuantizer
        std::cout << "\n--- Timing Breakdown ---" << std::endl;
        std::cout << "Average time for counting bit flips: " 
                  << pq.get_average_count_bit_flips_time() * 1e6 << " microseconds" << std::endl;
        std::cout << "Average time for encoding write data: " 
                  << pq.get_average_encoding_time() * 1e6 << " microseconds" << std::endl;
        std::cout << "Average time for calculating distance: " 
                  << pq.get_average_distance_calculation_time() * 1e6 << " microseconds" << std::endl;
        std::cout << "Average time for finding nearest page: " 
                  << pq.get_average_find_nearest_page_time() * 1e6 << " microseconds" << std::endl;

        // Cleanup
        munmap(pmem, PMEM_FILE_SIZE);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
