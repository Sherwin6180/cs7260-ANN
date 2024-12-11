// pq_algorithm.cpp
#include <vector>
#include <cstring>
#include <algorithm>
#include <random>
#include <iostream>
#include <limits>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <chrono> // Include for timing
#include "common.h" 

const size_t SUBVECTOR_SIZE = 16;  
const size_t NUM_CENTROIDS = 256; 
const size_t NUM_SUBVECTORS = PAGE_SIZE / SUBVECTOR_SIZE;

class ProductQuantizer {
private:
    // For each subvector position, store its centroids
    std::vector<std::vector<std::vector<uint8_t>>> centroids;  // [subvector_position][centroid_id][bytes]
    // For each page, store centroid indices for each subvector
    std::vector<std::vector<uint8_t>> encoded_pages;  // [page_id][subvector_indices]

    // Timing accumulators
    std::chrono::duration<double> total_count_bit_flips_time = std::chrono::duration<double>::zero();
    size_t count_bit_flips_total_calls = 0;

    std::chrono::duration<double> total_encoding_time = std::chrono::duration<double>::zero();
    size_t encoding_total_calls = 0;

    std::chrono::duration<double> total_distance_calculation_time = std::chrono::duration<double>::zero();
    size_t distance_calculation_total_calls = 0;

    std::chrono::duration<double> total_find_nearest_page_time = std::chrono::duration<double>::zero();
    size_t find_nearest_page_total_calls = 0;

    // Extract all subvectors at a specific position from all pages
    std::vector<std::vector<uint8_t>> random_pick_subvector_position(
        const uint8_t* pmem_data, size_t subvector_pos) {
        
        std::vector<std::vector<uint8_t>> subvectors;
        // Extract all subvectors at this position from all pages
        for (size_t page = 0; page < NUM_PAGES; page++) {
            const uint8_t* page_start = pmem_data + (page * PAGE_SIZE);
            const uint8_t* subvector_start = page_start + (subvector_pos * SUBVECTOR_SIZE);
            std::vector<uint8_t> subvector(subvector_start, subvector_start + SUBVECTOR_SIZE);
            subvectors.push_back(subvector);
        }

        // Randomly select centroids from existing subvectors
        std::vector<std::vector<uint8_t>> position_centroids;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, NUM_PAGES - 1);
        
        // Keep track of selected indices to avoid duplicates
        std::unordered_set<size_t> selected_indices;
        while (position_centroids.size() < NUM_CENTROIDS) {
            size_t idx = dis(gen);
            if (selected_indices.insert(idx).second) {
                position_centroids.push_back(subvectors[idx]);
            }
        }

        return position_centroids;
    }

    // K-means clustering for one subvector position
    std::vector<std::vector<uint8_t>> train_subvector_position(
        const uint8_t* pmem_data, size_t subvector_pos, const int max_iter = 100000) {

        int iter = 0;
        std::vector<std::vector<uint8_t>> subvectors;
        // Extract all subvectors at this position from all pages
        for (size_t page = 0; page < NUM_PAGES; page++) {
            const uint8_t* page_start = pmem_data + (page * PAGE_SIZE);
            const uint8_t* subvector_start = page_start + (subvector_pos * SUBVECTOR_SIZE);
            std::vector<uint8_t> subvector(subvector_start, subvector_start + SUBVECTOR_SIZE);
            subvectors.push_back(subvector);
        }

        // Initialize centroids randomly from existing subvectors
        std::vector<std::vector<uint8_t>> position_centroids;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, NUM_PAGES - 1);
        
        for (size_t i = 0; i < NUM_CENTROIDS; i++) {
            position_centroids.push_back(subvectors[dis(gen)]);
        }

        // Perform k-means clustering
        bool changed;
        do {
            changed = false;
            std::vector<std::vector<std::vector<uint8_t>>> clusters(NUM_CENTROIDS);
            
            // Assign subvectors to nearest centroids
            for (const auto& subvector : subvectors) {
                size_t best_centroid = 0;
                size_t min_distance = std::numeric_limits<size_t>::max();
                
                for (size_t c = 0; c < NUM_CENTROIDS; c++) {
                    size_t distance = count_bit_flips(
                        subvector.data(),
                        position_centroids[c].data(),
                        SUBVECTOR_SIZE
                    );
                    if (distance < min_distance) {
                        min_distance = distance;
                        best_centroid = c;
                    }
                }
                clusters[best_centroid].push_back(subvector);
            }

            // Update centroids
            for (size_t c = 0; c < NUM_CENTROIDS; c++) {
                if (clusters[c].empty()) continue;
                
                std::vector<uint8_t> new_centroid(SUBVECTOR_SIZE, 0);
                for (const auto& subvector : clusters[c]) {
                    for (size_t i = 0; i < SUBVECTOR_SIZE; i++) {
                        new_centroid[i] += subvector[i];
                    }
                }
                
                for (size_t i = 0; i < SUBVECTOR_SIZE; i++) {
                    new_centroid[i] /= clusters[c].size();
                }
                
                if (new_centroid != position_centroids[c]) {
                    changed = true;
                    position_centroids[c] = new_centroid;
                }
            }
            iter++;
            if(iter % 100 == 0) {
                printf("Iteration %d\n", iter);
            }
        } while (changed && iter < max_iter);

        return position_centroids;
    }

public:
    void train(const uint8_t* pmem_data, const int max_iter = 100000) {
        centroids.resize(NUM_SUBVECTORS);

        unsigned int num_threads = std::thread::hardware_concurrency() - 1;
        if (num_threads == 0) num_threads = 1;
        std::cout << "Training with " << num_threads << " threads" << std::endl;
        // Create threads and divide work
        std::vector<std::thread> threads;
        std::mutex cout_mutex; // For synchronized printing
        
        for (unsigned int t = 0; t < num_threads; t++) {
            threads.emplace_back([&, t]() {
                for (size_t pos = t; pos < NUM_SUBVECTORS; pos += num_threads) {
                    {
                        std::lock_guard<std::mutex> lock(cout_mutex);
                        std::cout << "Training subvector position " << pos 
                                << " on thread " << t << std::endl;
                    }
                    centroids[pos] = train_subvector_position(pmem_data, pos, max_iter);
                }
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
        
        
        // Encode all pages
        encoded_pages.resize(NUM_PAGES);
        for (size_t page = 0; page < NUM_PAGES; page++) {
            encoded_pages[page].resize(NUM_SUBVECTORS);
            const uint8_t* page_data = pmem_data + (page * PAGE_SIZE);
            
            for (size_t pos = 0; pos < NUM_SUBVECTORS; pos++) {
                const uint8_t* subvector = page_data + (pos * SUBVECTOR_SIZE);
                size_t best_centroid = 0;
                size_t min_distance = std::numeric_limits<size_t>::max();
                
                for (size_t c = 0; c < NUM_CENTROIDS; c++) {
                    size_t distance = count_bit_flips(
                        subvector,
                        centroids[pos][c].data(),
                        SUBVECTOR_SIZE
                    );
                    if (distance < min_distance) {
                        min_distance = distance;
                        best_centroid = c;
                    }
                }
                encoded_pages[page][pos] = best_centroid;
            }
        }
    }

    size_t find_nearest_page(const uint8_t* write_data) {
        auto start_find = std::chrono::high_resolution_clock::now();
        find_nearest_page_total_calls++;

        // Encoding the write data
        auto start_encoding = std::chrono::high_resolution_clock::now();
        std::vector<uint8_t> write_centroids(NUM_SUBVECTORS);
        
        for (size_t pos = 0; pos < NUM_SUBVECTORS; pos++) {
            const uint8_t* subvector = write_data + (pos * SUBVECTOR_SIZE);
            size_t best_centroid = 0;
            size_t min_distance = std::numeric_limits<size_t>::max();
            
            auto start_bit_flips = std::chrono::high_resolution_clock::now();
            for (size_t c = 0; c < NUM_CENTROIDS; c++) {
                size_t distance = count_bit_flips(
                    subvector,
                    centroids[pos][c].data(),
                    SUBVECTOR_SIZE
                );
                if (distance < min_distance) {
                    min_distance = distance;
                    best_centroid = c;
                }
            }
            auto end_bit_flips = std::chrono::high_resolution_clock::now();
            total_count_bit_flips_time += (end_bit_flips - start_bit_flips);
            count_bit_flips_total_calls += NUM_CENTROIDS;
            
            write_centroids[pos] = best_centroid;
        }
        auto end_encoding = std::chrono::high_resolution_clock::now();
        total_encoding_time += (end_encoding - start_encoding);
        encoding_total_calls++;

        // Finding the nearest page
        size_t best_page = 0;
        size_t min_distance = std::numeric_limits<size_t>::max();

        for (size_t page = 0; page < NUM_PAGES; page++) {
            size_t distance = 0;

            // Start timing the distance calculation loop
            auto start_distance_calc = std::chrono::high_resolution_clock::now();

            for (size_t pos = 0; pos < NUM_SUBVECTORS; pos++) {
                if (write_centroids[pos] != encoded_pages[page][pos]) {
                    distance++;
                }
            }

            auto end_distance_calc = std::chrono::high_resolution_clock::now();
            total_distance_calculation_time += (end_distance_calc - start_distance_calc);
            distance_calculation_total_calls++;

            if (distance < min_distance) {
                min_distance = distance;
                best_page = page;
            }
        }

        auto end_find = std::chrono::high_resolution_clock::now();
        total_find_nearest_page_time += (end_find - start_find);
        
        return best_page;
    }

    // Getter functions for average times
    double get_average_count_bit_flips_time() const { 
        return count_bit_flips_total_calls > 0 ? total_count_bit_flips_time.count() / count_bit_flips_total_calls : 0.0; 
    }

    double get_average_encoding_time() const { 
        return encoding_total_calls > 0 ? total_encoding_time.count() / encoding_total_calls : 0.0; 
    }

    double get_average_distance_calculation_time() const { 
        return distance_calculation_total_calls > 0 ? total_distance_calculation_time.count() / distance_calculation_total_calls : 0.0; 
    }

    double get_average_find_nearest_page_time() const { 
        return find_nearest_page_total_calls > 0 ? total_find_nearest_page_time.count() / find_nearest_page_total_calls : 0.0; 
    }
};
