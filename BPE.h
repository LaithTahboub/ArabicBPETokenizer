//
// Created by ltahboub on 2/19/25.
//

/*
    This file defines the BPE class structure.
*/

#ifndef BPE_H
#define BPE_H


#include <vector>
#include <string>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <optional>
#include <queue>

// fast hash function for pairs
struct PairHash {
    template <class T1, class T2>
    size_t operator()(const std::pair<T1, T2>& p) const {
        size_t h1 = std::hash<T1>{}(p.first);
        size_t h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1) ^ (h1 >> 1) ^ (h2 << 3);
    }
};

struct PairFrequency {
    std::pair<int, int> pair;
    int count;
    bool operator<(const PairFrequency& other) const { return count < other.count; }
};


class BPE {
    
private:
   // Use unordered_map for O(1) lookups during tokenization
   std::unordered_map<std::pair<int, int>, int, PairHash> merges;
   // Store merges in the order they were added
   std::vector<std::pair<std::pair<int, int>, int>> merge_order;

   std::vector<std::wstring> ids_to_tokens; // ID = vector index

   std::unordered_map<std::wstring, int> tokens_to_ids; // Faster lookups

   std::unordered_map<std::pair<int, int>, int, PairHash> pair_counts;

   std::priority_queue<PairFrequency> pair_queue;

   void save_vocab(const std::string& path);
   void save_merges(const std::string& path);
   void load_vocab(const std::string& path);
   void load_merges(const std::string& path);

   

public:

    void train(const std::wstring& text, int vocab_size, const std::unordered_set<std::wstring>& allowed_special);
    std::vector<int> encode(const std::wstring& text);
    std::vector<int> tokenize(const std::wstring& token);
    std::wstring decode(const std::vector<int>& token_ids);
    void save(const std::string& path);
    void load(const std::string& path);

    
   

    std::vector<std::wstring> split_on_space_or_newline(const std::wstring& input) {
        std::vector<std::wstring> tokens;
        size_t start = 0, end = 0;
        
        while ((end = input.find_first_of(L" \n", start)) != std::wstring::npos) {
            if (end != start) // Skip empty tokens
                tokens.emplace_back(input.substr(start, end - start));
            start = end + 1;
        }
        if (start < input.length())
            tokens.emplace_back(input.substr(start));
        
        return tokens;
    }


    std::wstring get_vocab() {
        std::wstring vocab;
        vocab.reserve(ids_to_tokens.size() * 20);  // Pre-allocate
        
        // Iterate by ID order using vector
        for (size_t id = 0; id < ids_to_tokens.size(); ++id) {
            vocab += ids_to_tokens[id] + L" -> " + std::to_wstring(id) + L"\n";
        }
        
        return vocab;
    }
};


#endif //LTAHBOUB_BPE_H
