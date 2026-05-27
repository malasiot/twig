#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

struct FormView {
    std::string id_;
    std::string name_;
    std::string full_name_;
    std::string value;
    std::string label_;
    std::vector<std::string> block_prefixes_; 
    std::vector<std::string> errors_;
    std::map<std::string, std::string> attributes_;
    
    bool is_rendered_ = false;
    
     std::vector<std::shared_ptr<FormView>> children;
};