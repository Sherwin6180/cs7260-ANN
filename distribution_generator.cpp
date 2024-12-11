#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <random>
#include <cmath>
#include <stdexcept>

// Function to generate a random string
std::string generate_random_string(size_t length) {
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

// Function to generate uniform distribution
void generate_uniform(const std::string& filename, size_t num_records, size_t value_length) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    for (size_t i = 0; i < num_records; ++i) {
        file << "key" << i << "," << generate_random_string(value_length) << "\n";
    }
    file.close();
    std::cout << "Uniform distribution written to " << filename << std::endl;
}

// Function to generate zipfian distribution
void generate_zipfian(const std::string& filename, size_t num_records, size_t value_length, double s = 1.0) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    double total = 0.0;
    for (size_t i = 1; i <= num_records; ++i) {
        total += 1.0 / std::pow(i, s);
    }

    std::default_random_engine rng(std::random_device{}());
    std::uniform_real_distribution<> dist(0.0, total);

    for (size_t i = 0; i < num_records; ++i) {
        double random_value = dist(rng);
        double cumulative = 0.0;
        size_t chosen_key = 0;

        for (size_t j = 1; j <= num_records; ++j) {
            cumulative += 1.0 / std::pow(j, s);
            if (cumulative >= random_value) {
                chosen_key = j - 1;
                break;
            }
        }

        file << "key" << chosen_key << "," << generate_random_string(value_length) << "\n";
    }

    file.close();
    std::cout << "Zipfian distribution written to " << filename << std::endl;
}

// Function to generate latest distribution
void generate_latest(const std::string& filename, size_t num_records, size_t value_length) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, num_records - 1);

    for (size_t i = 0; i < num_records; ++i) {
        size_t recent_key_index = dist(rng) % 100;
        file << "key" << recent_key_index << "," << generate_random_string(value_length) << "\n";
    }

    file.close();
    std::cout << "Latest distribution written to " << filename << std::endl;
}

// Function to generate hotspot distribution
void generate_hotspot(const std::string& filename, size_t num_records, size_t value_length, double hotspot_fraction = 0.2, double hotspot_op_fraction = 0.8) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t hotspot_size = num_records * hotspot_fraction;
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> hotspot_dist(0, hotspot_size - 1);
    std::uniform_int_distribution<> coldspot_dist(hotspot_size, num_records - 1);
    std::bernoulli_distribution is_hotspot(hotspot_op_fraction);

    for (size_t i = 0; i < num_records; ++i) {
        size_t key_index = is_hotspot(rng) ? hotspot_dist(rng) : coldspot_dist(rng);
        file << "key" << key_index << "," << generate_random_string(value_length) << "\n";
    }

    file.close();
    std::cout << "Hotspot distribution written to " << filename << std::endl;
}

// Entry point for the program
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <distribution> <output_file> [optional parameters]\n";
        std::cerr << "Supported distributions: uniform, zipfian, latest, hotspot\n";
        return 1;
    }

    std::string distribution = argv[1];
    std::string filename = argv[2];

    const size_t num_records = 100000; // Default number of records
    const size_t value_length = 100;   // Default value length

    try {
        if (distribution == "uniform") {
            generate_uniform(filename, num_records, value_length);
        } else if (distribution == "zipfian") {
            double s = (argc >= 4) ? std::stod(argv[3]) : 1.0; // Zipfian parameter (optional)
            generate_zipfian(filename, num_records, value_length, s);
        } else if (distribution == "latest") {
            generate_latest(filename, num_records, value_length);
        } else if (distribution == "hotspot") {
            double hotspot_fraction = (argc >= 4) ? std::stod(argv[3]) : 0.2;   // Optional parameter
            double hotspot_op_fraction = (argc >= 5) ? std::stod(argv[4]) : 0.8; // Optional parameter
            generate_hotspot(filename, num_records, value_length, hotspot_fraction, hotspot_op_fraction);
        } else {
            std::cerr << "Unknown distribution: " << distribution << "\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
