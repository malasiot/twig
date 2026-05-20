#ifndef TWIG_DATE_HELPERS_HPP
#define TWIG_DATE_HELPERS_HPP

#include <cstdint>
#include <ctime>
#include <regex>
#include <string>
#include <vector>

namespace twig {

std::string php_date_format_to_strftime(const std::string &format) ;

bool parse_fixed_date_formats(const std::string &src, std::tm &tm) ;

bool apply_relative_offset(int64_t &base_s, int64_t amount, const std::string &unit) ;

int64_t strtotime(const std::string &src, const std::string &tz = "") ;

bool parse_timezone_offset(const std::string &tz, int &offset_seconds) ;

}

#endif
