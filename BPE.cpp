//
// Created by ltahboub on 2/19/25.
//

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

// BPE::BPE() {
//     merges = std::map<std::pair<int, int>, int>();
//     ids_to_tokens = std::map<int, std::wstring>();
//     tokens_to_ids = std::map<std::wstring, int>();

// }

void BPE::train(const std::wstring& text, int vocab_size, const std::unordered_set<std::wstring>& allowed_special) {

    // Debgugging file: merges.txt, used currently for looking at merges
    std::wofstream file2("merges.txt", std::ios::binary);
    file2.imbue(std::locale(file2.getloc(), new std::codecvt_utf8<wchar_t>));
    
    if (!file2.is_open()) {
        throw std::runtime_error("Unable to open file for writing: merges.txt");
    }



    // Initialize ASCII (0-255)
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

    std::vector<int> token_ids;
    
    for (const wchar_t& ch : text) {
        std::wstring ch_str(1, ch);
        auto it = tokens_to_ids.find(ch_str);
        if (it == tokens_to_ids.end()) {
            throw std::runtime_error("Character not found in vocab.");
        }
        token_ids.emplace_back(it->second);
    }
    for (int new_id = tokens_to_ids.size(); new_id < vocab_size; ++new_id) {
        auto pair_opt = find_freq_pair(token_ids);
        if (!pair_opt.has_value()) break;

        auto pair_id = pair_opt.value();
        replace_pair_inplace(token_ids, pair_id, new_id);
        
        // Add to both merges and merge_order
        merges[pair_id] = new_id;
        merge_order.emplace_back(pair_id, new_id); // Store insertion order
    }

    // Process merges in insertion order (no sorting needed)
    for (const auto& merge : merge_order) {
        const auto& pair_ids = merge.first;
        int new_id = merge.second;

        std::wstring merged_token = ids_to_tokens[pair_ids.first] + ids_to_tokens[pair_ids.second];
        ids_to_tokens[new_id] = merged_token;
        tokens_to_ids[merged_token] = new_id;
    // Optional: Write to vocab file for debugging
    // std::wstring vocab_line = decode({pair_ids.first}) + L" + " +
    //                           decode({pair_ids.second}) + L" = " +
    //                           decode({new_id});
    // file2 << vocab_line << L"\n";
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

int main() {


    std::string file_path = "quran.txt";

    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Unable to open file: " + file_path);
    }

    // Quickly get file size and preallocate buffer
    auto file_size = file.tellg();
    // std::cout << file_size;
    file.seekg(0, std::ios::beg);
    std::string utf8_content(file_size, '\0');

    // Fast read entire file into memory
    if (!file.read(&utf8_content[0], file_size)) {
        throw std::runtime_error("Error reading file: " + file_path);
    }

    file.close();

    // // Convert UTF-8 to wide string efficiently (single pass)
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring quran_txt = converter.from_bytes(utf8_content);

    // // Set locale for Unicode output
    std::locale::global(std::locale(""));
    std::wcout.imbue(std::locale());

    // // Important: set locale for wcout
    std::locale::global(std::locale(""));
    std::wcout.imbue(std::locale());

    BPE *bpe = new BPE;
    

    bpe->train(quran_txt, 6000, std::unordered_set<std::wstring>());

    // // std::cout << "[";
    // // for (int token : bpe->encode("الَّذِينَ"))
    // //     std::cout << token << ", ";
    // // std::cout << "]"; 


    // std::wstring vocab = bpe->get_vocab();

    // std::wofstream file2("vocab.txt", std::ios::binary);
    // file2.imbue(std::locale(file2.getloc(), new std::codecvt_utf8<wchar_t>));

    
    // if (!file2.is_open()) {
    //     throw std::runtime_error("Unable to open file for writing: vocab.txt");
    // }

    // file2 << vocab;



    return 0;
}