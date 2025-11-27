#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <string>

class DualLogger {
public:
    DualLogger(const std::string& log_file_path) {
        // Ensure parent folder exists
        std::filesystem::path p(log_file_path);
        std::filesystem::create_directories(p.parent_path());

        file.open(p, std::ios::out | std::ios::app);
        if(!file.is_open()) {
            std::cerr << "[DualLogger ERROR] Failed to open log file: " << log_file_path << std::endl;
            throw std::runtime_error("Failed to open log file");
        }
    }

    ~DualLogger() {
        std::lock_guard<std::mutex> lock(mtx);
        if(file.is_open()) file.close();
    }

    // Log message to both terminal and file
    void log(const std::string& msg, bool to_terminal=true, bool to_file=true) {
        std::lock_guard<std::mutex> lock(mtx);
        if(to_terminal) {
            std::cout << msg << std::endl;
        }
        if(to_file && file.is_open()) {
            file << msg << std::endl;
            file.flush(); // Ensure it writes immediately
        }
    }

    // Convenience wrappers
    void info(const std::string& msg, bool to_terminal=true, bool to_file=true) { log("[INFO] " + msg, to_terminal, to_file); }
    void warn(const std::string& msg, bool to_terminal=true, bool to_file=true) { log("[WARN] " + msg, to_terminal, to_file); }
    void error(const std::string& msg, bool to_terminal=true, bool to_file=true) { log("[ERROR] " + msg, to_terminal, to_file); }

private:
    std::ofstream file;
    std::mutex mtx;
};
