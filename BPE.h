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

// fast hash function for pairs
struct PairHash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& pair) const {
        std::size_t h1 = std::hash<T1>{}(pair.first);
        std::size_t h2 = std::hash<T2>{}(pair.second);
        return h1 ^ (h2 << 1);
    }
};

class BPE {
    
private:
    std::map<std::pair<int, int>, int> merges;

    std::map<int, std::wstring> ids_to_tokens;

public:


    std::map<std::wstring, int> tokens_to_ids;

    void train(const std::wstring& text, int vocab_size, const std::unordered_set<std::wstring>& allowed_special);
    std::vector<int> encode(const std::wstring& text);
    std::vector<int> tokenize(const std::wstring& token);
    std::wstring decode(const std::vector<int>& token_ids);



    // helper function to find most frequent pair from token_ids
    std::optional<std::pair<int, int>> find_freq_pair(const std::vector<int>& token_ids) {
        if (token_ids.size() < 2)
            return std::nullopt;
    
        std::unordered_map<std::pair<int, int>, int, PairHash> pairs;
    
        std::pair<int,int> max_pair = {0,0};
        int max_count = 0;
    
        for (size_t i = 0; i < token_ids.size() - 1; ++i) {
            auto pair = std::make_pair(token_ids[i], token_ids[i + 1]);
            int count = ++pairs[pair];
            if (count > max_count) {
                max_count = count;
                max_pair = pair;
            }
        }
    
        return max_count > 1 ? std::optional(max_pair) : std::nullopt;
    }

    // helper to replace pair in merges
    void replace_pair_inplace(std::vector<int>& token_ids, const std::pair<int, int>& pair_id, int new_id) {
        size_t dst = 0, src = 0;
        size_t n = token_ids.size();
    
        while (src < n) {
            if (src < n - 1 && token_ids[src] == pair_id.first && token_ids[src + 1] == pair_id.second) {
                token_ids[dst++] = new_id;
                src += 2; // Skip the pair
            } else {
                token_ids[dst++] = token_ids[src++];
            }
        }
        token_ids.resize(dst); // resize once, minimal overhead
    }


    std::vector<std::wstring> split_on_space_or_newline(const std::wstring& input) {
        std::vector<std::wstring> tokens;
        std::wistringstream stream(input);
        std::wstring token;
    
        while (stream >> token) {
            tokens.push_back(token);
        }
    
        return tokens;
    }


    std::wstring get_vocab() {
        std::wstring vocab;
        for (auto p : tokens_to_ids) {
            std::string arrow = " -> ";
            std::string newline = "\n";
            vocab += p.first + std::wstring(arrow.begin(), arrow.end()) + std::to_wstring(p.second) + std::wstring(newline.begin(), newline.end()) ;
        }
        return vocab;
    }
};


#endif //LTAHBOUB_BPE_H
