//
// Created by ltahboub on 2/19/25.
//

#include <filesystem> 

#include "BPE.h"

#include <vector>
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <optional>
#include <locale>
#include <codecvt>
#include <string>
#include <queue> 

namespace fs = std::filesystem;




void BPE::train(const std::wstring& text, int vocab_size, const std::unordered_set<std::wstring>& allowed_special) {

    // Debgugging file: merges.txt, used currently for looking at merges
    std::wofstream file2("merges.txt", std::ios::binary);
    file2.imbue(std::locale(file2.getloc(), new std::codecvt_utf8<wchar_t>));
    
    if (!file2.is_open()) {
        throw std::runtime_error("Unable to open file for writing: merges.txt");
    }



    // Initialize ASCII (0-255). TODO, change this to add arabic letters in the beginning
    ids_to_tokens.reserve(256 + 4096);  // Pre-allocate for ASCII + expected merges
    for (int i = 0; i < 256; ++i) {
        wchar_t ch = static_cast<wchar_t>(i);
        ids_to_tokens.emplace_back(1, ch);  // Direct construction
        tokens_to_ids[ids_to_tokens.back()] = i;
    }

    // Add distinct text characters
    std::unordered_set<wchar_t> distinct_chars(text.begin(), text.end());
    for (wchar_t ch : distinct_chars) {
        std::wstring s(1, ch);
        if (!tokens_to_ids.count(s)) {
            ids_to_tokens.emplace_back(s);
            tokens_to_ids[s] = ids_to_tokens.size() - 1;  // ID = current size
        }
    }

    // Add special tokens
    for (const auto& tok : allowed_special) {
        if (!tokens_to_ids.count(tok)) {
            ids_to_tokens.emplace_back(tok);
            tokens_to_ids[tok] = ids_to_tokens.size() - 1;
        }
    }

    std::vector<std::vector<int>> word_token_ids;
    std::vector<std::wstring> words = split_on_space_or_newline(text);
    for (const auto& word : words) {
        std::vector<int> ids;
        for (wchar_t ch : word) {
            std::wstring ch_str(1, ch);
            ids.push_back(tokens_to_ids.at(ch_str)); //  (O(1))
        }
        word_token_ids.push_back(ids);
    }
    
   
    // Initialize pair frequencies
    for (const auto& word : word_token_ids) {
        for (size_t i = 0; i < word.size() - 1; ++i) {
            auto pair = std::make_pair(word[i], word[i + 1]);
            pair_counts[pair]++;
        }
    }
    for (const auto& [pair, count] : pair_counts) {
        pair_queue.push({pair, count});
    }
    
    // Merge loop using priority queue
    while (!pair_queue.empty() && ids_to_tokens.size() < vocab_size) {
        auto current = pair_queue.top();
        pair_queue.pop();
    
        // Skip if frequency is outdated
        if (current.count != pair_counts[current.pair]) continue;
    
        int new_id = ids_to_tokens.size();
        merges[current.pair] = new_id;
        merge_order.emplace_back(current.pair, new_id);
    
        // Update vocabulary
        std::wstring merged_token = ids_to_tokens[current.pair.first] + ids_to_tokens[current.pair.second];
        ids_to_tokens.push_back(merged_token);
        tokens_to_ids[merged_token] = new_id;
    
        // Process each word to apply merges and update frequencies
        for (auto& word : word_token_ids) {
            size_t i = 0;
            while (i < word.size() - 1) {
                if (word[i] == current.pair.first && word[i + 1] == current.pair.second) {
                    // Remove old pairs from global counts
                    if (i > 0) {
                        auto left_pair = std::make_pair(word[i - 1], word[i]);
                        pair_counts[left_pair]--;
                    }
                    if (i < word.size() - 2) {
                        auto right_pair = std::make_pair(word[i + 1], word[i + 2]);
                        pair_counts[right_pair]--;
                    }
    
                    // Replace with merged token
                    word[i] = new_id;
                    word.erase(word.begin() + i + 1);
    
                    // Add new pairs to global counts
                    if (i > 0) {
                        auto new_left_pair = std::make_pair(word[i - 1], word[i]);
                        pair_counts[new_left_pair]++;
                        pair_queue.push({new_left_pair, pair_counts[new_left_pair]});
                    }
                    if (i < word.size() - 1) {
                        auto new_right_pair = std::make_pair(word[i], word[i + 1]);
                        pair_counts[new_right_pair]++;
                        pair_queue.push({new_right_pair, pair_counts[new_right_pair]});
                    }
                } else {
                    i++;
                }
            }
        }
    }
    // Process merges in insertion order (no sorting needed)
    for (const auto& merge : merge_order) {
        const auto& pair_ids = merge.first;
        int new_id = merge.second;

        std::wstring merged_token = ids_to_tokens[pair_ids.first] + ids_to_tokens[pair_ids.second];
        ids_to_tokens[new_id] = merged_token;
        tokens_to_ids[merged_token] = new_id;
    // Optional: Write to vocab file for debugging
    std::wstring vocab_line = decode({pair_ids.first}) + L" + " +
                              decode({pair_ids.second}) + L" = " +
                              decode({new_id});
    file2 << vocab_line << L"\n";
    }
    

}


std::vector<int> BPE::encode(const std::wstring& text) {
    // this takes in a wstring and returns the list of tokens ids 

    std::vector<std::wstring> tokens = split_on_space_or_newline(text);

    // TODO: Think about ensuring that \n is treated as a separate token
    // std::vector<std::wstring> words = text.replace("\n", " \n ").split()

    std::vector<int> token_ids;
    for (const std::wstring& token : tokens)
        if (tokens_to_ids.find(token) != tokens_to_ids.end()) {
            // token is contained in the vocabulary as is
            int token_id = tokens_to_ids[token];
            token_ids.emplace_back(token_id);
        }
        else {
            // try to handle subword tokenization via BPE
            std::vector<int> sub_token_ids = tokenize(token);
            
            for (int tk : sub_token_ids) {
                token_ids.emplace_back(tk);
            }
        }

    return token_ids;
}


std::vector<int> BPE::tokenize(const std::wstring& token) {
    std::vector<int> token_ids;
    token_ids.reserve(token.length());  // Pre-allocate
    
    for (wchar_t ch : token) {
        // Fast path for ASCII (85%+ of typical Arabic text)
        if (static_cast<unsigned>(ch) < 256) {
            token_ids.push_back(static_cast<int>(ch));
        } 
        // Lookup for non-ASCII
        else if (auto it = tokens_to_ids.find(std::wstring(1, ch)); 
                 it != tokens_to_ids.end()) {
            token_ids.push_back(it->second);
        }
        else {
            throw std::runtime_error("Unknown character");
        }
    }

    bool can_merge = true;
    while (can_merge && token_ids.size() > 1) {
        can_merge = false;
        std::vector<int> new_tokens;
        size_t i = 0;

        while (i < token_ids.size() - 1) {
            std::pair<int, int> pair = {token_ids[i], token_ids[i + 1]};
            auto merge_it = merges.find(pair);

            if (merge_it != merges.end()) {
                int merged_token_id = merge_it->second;
                new_tokens.push_back(merged_token_id);
                // Uncomment for debugging or educational purposes
                // std::cout << "Merged pair {" << pair.first << ", " << pair.second << "} -> " << merged_token_id << " ('" << ids_to_tokens[merged_token_id] << "')" << std::endl;
                i += 2; // Skip next token as it's merged
                can_merge = true;
            } else {
                new_tokens.push_back(token_ids[i]);
                ++i;
            }
        }

        // Add the last token if not already processed
        if (i < token_ids.size()) {
            new_tokens.push_back(token_ids[i]);
        }

        token_ids = new_tokens;
    }

    return token_ids;
}

std::wstring BPE::decode(const std::vector<int>& token_ids) {
    std::wstring result;
    result.reserve(token_ids.size() * 2);  // Estimate average token length
    
    for (int id : token_ids) {
        // Direct vector access: O(1)
        if (id < 0 || id >= static_cast<int>(ids_to_tokens.size())) {
            throw std::invalid_argument("Invalid token ID");
        }
        result += ids_to_tokens[id];
    }
    
    return result;
}


// Helper function to split command line arguments
std::vector<std::wstring> split_command(const std::wstring& input) {
    std::vector<std::wstring> args;
    std::wstring arg;
    bool in_quotes = false;
    
    for (wchar_t c : input) {
        if (c == L'"') {
            in_quotes = !in_quotes;
        } else if (isspace(c) && !in_quotes) {
            if (!arg.empty()) {
                args.push_back(arg);
                arg.clear();
            }
        } else {
            arg += c;
        }
    }
    
    if (!arg.empty()) {
        args.push_back(arg);
    }
    
    return args;
}


/* File IO Business */


void BPE::save(const std::string& path) {
    fs::create_directories(path);
    save_vocab((fs::path(path) / "vocab.txt").string());
    save_merges((fs::path(path) / "merges.txt").string());
}

void BPE::load(const std::string& path) {
    load_vocab((fs::path(path) / "vocab.txt").string());
    load_merges((fs::path(path) / "merges.txt").string());
}

void BPE::save_vocab(const std::string& path) {
    std::wofstream file(path);
    file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
    
    for (size_t id = 0; id < ids_to_tokens.size(); ++id) {
        file << id << L" " << ids_to_tokens[id] << L"\n";
    }
}

void BPE::save_merges(const std::string& path) {
    std::wofstream file(path);
    file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
    
    for (const auto& merge : merge_order) {
        file << merge.first.first << L" " 
             << merge.first.second << L" " 
             << merge.second << L"\n";
    }
}

void BPE::load_vocab(const std::string& path) {
    std::wifstream file(path);
    file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
    
    ids_to_tokens.clear();
    tokens_to_ids.clear();
    
    std::wstring line;
    size_t line_num = 0;
    
    while (std::getline(file, line)) {
        line_num++;
        if (line.empty()) continue;
        
        size_t space_pos = line.find(L' ');
        if (space_pos == std::wstring::npos) {
            throw std::runtime_error("Invalid vocab format at line " + 
                                   std::to_string(line_num));
        }
        
        try {
            // Extract ID
            int id = std::stoi(line.substr(0, space_pos));
            
            // Validate ID order
            if (id != static_cast<int>(ids_to_tokens.size())) {
                throw std::runtime_error("Vocab ID mismatch at line " + 
                                       std::to_string(line_num) + 
                                       ". Expected ID: " + 
                                       std::to_string(ids_to_tokens.size()) + 
                                       ", Found: " + std::to_string(id));
            }
            
            // Extract token
            std::wstring token = line.substr(space_pos + 1);
            
            ids_to_tokens.push_back(token);
            tokens_to_ids[token] = id;
            
        } catch (const std::invalid_argument&) {
            throw std::runtime_error("Non-integer ID at line " + 
                                   std::to_string(line_num));
        } catch (const std::out_of_range&) {
            throw std::runtime_error("ID out of range at line " + 
                                   std::to_string(line_num));
        }
    }
}

void BPE::load_merges(const std::string& path) {
    std::wifstream file(path);
    file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
    
    merges.clear();
    merge_order.clear();
    
    std::wstring line;
    while (std::getline(file, line)) {
        std::wistringstream iss(line);
        int left, right, new_id;
        iss >> left >> right >> new_id;
        
        auto pair = std::make_pair(left, right);
        merges[pair] = new_id;
        merge_order.emplace_back(pair, new_id);
    }
}

int main() {
    BPE bpe;
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    
    std::cout << "BPE Tokenizer Shell\nType 'quit' or 'exit' to exit\n";
    
    while (true) {
        std::cout << "> ";
        std::string input;
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        
        std::vector<std::string> args;
        std::istringstream iss(input);
        std::string token;
        bool in_quotes = false;
        std::string current;
        
        // Improved argument parsing
        for (char c : input) {
            if (c == '"') {
                in_quotes = !in_quotes;
            } else if (isspace(c) && !in_quotes) {
                if (!current.empty()) {
                    args.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        if (!current.empty()) {
            args.push_back(current);
        }

        try {
            if (args.empty()) continue;
            
            if (args[0] == "quit" || args[0] == "exit") {
                break;
            }
            else if (args[0] == "load") {
                if (args.size() != 2) throw std::invalid_argument("Usage: load <directory>");
                bpe.load(args[1]);
                std::cout << "Loaded model from " << args[1] << "\n";
            }
            else if (args[0] == "train") {
                if (args.size() != 4) throw std::invalid_argument("Usage: train <text_file> <vocab_size> <save_dir>");
                
                // Read training file
                std::ifstream file(args[1]);
                std::string content((std::istreambuf_iterator<char>(file)), 
                    std::istreambuf_iterator<char>());
                std::wstring text = converter.from_bytes(content);
                
                // Train and save
                bpe.train(text, std::stoi(args[2]), {});
                bpe.save(args[3]);
                std::cout << "Trained and saved model to " << args[3] << "\n";
            }
            else if (args[0] == "encode") {
                if (args.size() != 2) throw std::invalid_argument("Usage: encode \"<text>\"");
                std::wstring wtext = converter.from_bytes(args[1]);
                auto ids = bpe.encode(wtext);
                
                std::cout << "[";
                for (size_t i = 0; i < ids.size(); ++i) {
                    std::cout << ids[i];
                    if (i != ids.size() - 1) std::cout << ", ";
                }
                std::cout << "]\n";
            }
            else if (args[0] == "decode") {
                if (args.size() != 2) throw std::invalid_argument("Usage: decode <comma_separated_ids>");
                
                std::vector<int> ids;
                std::stringstream ss(args[1]);
                std::string token;
                
                while (std::getline(ss, token, ',')) {
                    ids.push_back(std::stoi(token));
                }
                
                std::wstring decoded = bpe.decode(ids);
                std::cout << converter.to_bytes(decoded) << "\n";
            }
            else {
                std::cout << "Unknown command: " << args[0] << "\n";
            }
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    
    return 0;
}