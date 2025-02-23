//
// Created by ltahboub on 2/19/25.
//

#include "BPE.h"

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>


// BPE::BPE() {
//     merges = std::map<std::pair<int, int>, int>();
//     ids_to_tokens = std::map<int, std::string>();
//     tokens_to_ids = std::map<std::string, int>();

// }

void BPE::train(std::string text, int vocab_size, std::set<std::string> allowed_special) {


    // init unique chars and add first 256 ascii chars
    std::vector<char> unique_chars;

    for (int i = 0; i < 256; ++i) {
        unique_chars.emplace_back(static_cast<char>(i));
    }


    // add distinct chars from text to unique chars
    std::set<char> sorted_set(text.begin(), text.end());  

    for (char c : sorted_set) {

        if (std::find(unique_chars.begin(), unique_chars.end(), c) == unique_chars.end()) {
            unique_chars.emplace_back(c);
        }
    }

    std::vector<int> token_ids;

    if (!allowed_special.empty()) {
        for (std::string token : allowed_special) {
            if (tokens_to_ids[token]) {
                int token_id = tokens_to_ids[token];
                token_ids.emplace_back(token_id);
            }
            else {
                // token doesn't exist in vocab, try to tokenize and get subwords
                std::vector<int> sub_token_ids = tokenize(token);
                for (int sub_token : sub_token_ids)
                    token_ids.emplace_back(sub_token);
            }
        }
    }



}


std::vector<int> BPE::encode(std::string text) {
    std::vector<std::string> tokens;
    // std::vector<std::string> words = text.replace("\n", " \n ").split()
}


std::vector<int> BPE::tokenize(std::string token) {
    // retrieve initial tokenization. should just be characters at very beginning
    std::vector<int> token_ids;
    for (std::pair<int, std::string> mapping : ids_to_tokens) {
        
    }


}

int main() {
    

    return 0;
}