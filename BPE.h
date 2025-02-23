//
// Created by ltahboub on 2/19/25.
//

#ifndef BPE_H
#define BPE_H


#include <vector>
#include <string>
#include <map>
#include <set>

class BPE {
private:
    std::map<std::pair<int, int>, int> merges;

    std::map<int, std::string> ids_to_tokens;

    std::map<std::string, int> tokens_to_ids;

public:


    // TODO: think about what type allowed_special should be, leaving as std::set for now 
    void BPE::train(std::string text, int vocab_size, std::set<std::string> allowed_special);
    std::vector<int> BPE::encode(std::string text);
    std::vector<int> BPE::tokenize(std::string token);
    std::string BPE::decode(std::vector<int> token_ids);
};


#endif //LTAHBOUB_BPE_H
