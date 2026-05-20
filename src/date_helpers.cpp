#include <twig/date_helpers.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <regex>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace twig {

static std::string to_lower_copy(const std::string &src) {
    std::string tmp = src ;
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), [](unsigned char c){ return std::tolower(c); });
    return tmp ;
}

std::string php_date_format_to_strftime(const std::string &format) {
    static const std::map<char, std::string> token_map = {
        {'d', "%d"}, {'D', "%a"}, {'j', "%-d"}, {'l', "%A"}, {'F', "%B"},
        {'m', "%m"}, {'M', "%b"}, {'n', "%-m"}, {'Y', "%Y"}, {'y', "%y"},
        {'H', "%H"}, {'h', "%I"}, {'G', "%-H"}, {'g', "%-I"},
        {'i', "%M"}, {'s', "%S"}, {'a', "%p"}, {'A', "%p"}, {'U', "%s"}
    };

    std::string result ;
    result.reserve(format.size() * 2);

    for ( size_t i = 0 ; i < format.size() ; ++i ) {
        char c = format[i] ;
        if ( c == '\\' && i + 1 < format.size() ) {
            result.push_back(format[++i]) ;
            continue ;
        }

        auto it = token_map.find(c) ;
        if ( it != token_map.end() ) {
            result.append(it->second) ;
        } else {
            result.push_back(c) ;
        }
    }

    return result ;
}

bool parse_fixed_date_formats(const std::string &src, std::tm &tm) {
    static const std::vector<std::string> formats = {
        "%Y-%m-%d %H:%M:%S",
        "%Y-%m-%d %H:%M",
        "%Y-%m-%d",
        "%Y/%m/%d %H:%M:%S",
        "%Y/%m/%d %H:%M",
        "%Y/%m/%d",
        "%d.%m.%Y %H:%M:%S",
        "%d.%m.%Y %H:%M",
        "%d.%m.%Y",
        "%b %d %Y %H:%M:%S",
        "%b %d %Y %H:%M",
        "%b %d %Y",
        "%B %d %Y %H:%M:%S",
        "%B %d %Y %H:%M",
        "%B %d %Y"
    };

    for ( const auto &fmt: formats ) {
        std::memset(&tm, 0, sizeof(tm));
        char *end = strptime(src.c_str(), fmt.c_str(), &tm) ;
        if ( end && *end == '\0' ) {
            tm.tm_isdst = -1 ;
            return true ;
        }
    }

    return false ;
}

static bool is_iana_timezone(const std::string &tz) {
    static const std::regex iana_re(R"(^[A-Za-z_]+/[A-Za-z0-9_]+(?:/[A-Za-z0-9_]+)?$)") ;
    return std::regex_match(tz, iana_re) ;
}

static int64_t mktime_with_tz(std::tm &tm, const std::string &tz) {
    tm.tm_isdst = -1 ;
    if ( tz.empty() ) {
        return static_cast<int64_t>(std::mktime(&tm)) ;
    }

    if ( is_iana_timezone(tz) ) {
        const char *original_tz = std::getenv("TZ") ;
        std::string saved_tz = original_tz ? original_tz : "" ;
        if ( setenv("TZ", tz.c_str(), 1) != 0 )
            return -1 ;
        tzset() ;
        int64_t result = static_cast<int64_t>(std::mktime(&tm)) ;
        if ( saved_tz.empty() )
            unsetenv("TZ") ;
        else
            setenv("TZ", saved_tz.c_str(), 1) ;
        tzset() ;
        return result ;
    }

    int offset_seconds ;
    if ( !parse_timezone_offset(tz, offset_seconds) )
        return -1 ;

    const char *original_tz = std::getenv("TZ") ;
    std::string saved_tz = original_tz ? original_tz : "" ;
    if ( setenv("TZ", "UTC", 1) != 0 )
        return -1 ;
    tzset() ;
    int64_t result = static_cast<int64_t>(std::mktime(&tm)) ;
    if ( saved_tz.empty() )
        unsetenv("TZ") ;
    else
        setenv("TZ", saved_tz.c_str(), 1) ;
    tzset() ;
    return result - offset_seconds ;
}

bool apply_relative_offset(int64_t &base_s, int64_t amount, const std::string &unit) {
    auto tp = std::chrono::system_clock::time_point(std::chrono::seconds(base_s)) ;
    std::string lower_unit = to_lower_copy(unit) ;

    if ( lower_unit == "second" || lower_unit == "seconds" ) {
        tp += std::chrono::seconds(amount) ;
    } else if ( lower_unit == "minute" || lower_unit == "minutes" ) {
        tp += std::chrono::minutes(amount) ;
    } else if ( lower_unit == "hour" || lower_unit == "hours" ) {
        tp += std::chrono::hours(amount) ;
    } else if ( lower_unit == "day" || lower_unit == "days" ) {
        tp += std::chrono::hours(amount * 24) ;
    } else if ( lower_unit == "week" || lower_unit == "weeks" ) {
        tp += std::chrono::hours(amount * 24 * 7) ;
    } else if ( lower_unit == "month" || lower_unit == "months" ) {
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()) ;
        std::time_t base = static_cast<std::time_t>(seconds.count()) ;
        std::tm tm = *std::localtime(&base) ;
        tm.tm_mon += static_cast<int>(amount) ;
        tm.tm_isdst = -1 ;
        base = std::mktime(&tm) ;
        base_s = static_cast<int64_t>(base) ;
        return true ;
    } else if ( lower_unit == "year" || lower_unit == "years" ) {
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()) ;
        std::time_t base = static_cast<std::time_t>(seconds.count()) ;
        std::tm tm = *std::localtime(&base) ;
        tm.tm_year += static_cast<int>(amount) ;
        tm.tm_isdst = -1 ;
        base = std::mktime(&tm) ;
        base_s = static_cast<int64_t>(base) ;
        return true ;
    } else {
        return false ;
    }

    base_s = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count() ;
    return true ;
}

bool parse_timezone_offset(const std::string &tz, int &offset_seconds) {
    if ( tz.empty() ) {
        offset_seconds = 0 ;
        return true ;
    }

    std::string upper = tz ;
    std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c){ return std::toupper(c); });
    if ( upper == "UTC" || upper == "GMT" ) {
        offset_seconds = 0 ;
        return true ;
    }

    // Try numeric offset format: +05:30, -08:00, +0530, etc.
    std::regex offset_re(R"(^([+-]?)(\d{1,2})(?::?(\d{2}))?$)") ;
    std::smatch match ;
    if ( std::regex_match(tz, match, offset_re) ) {
        int sign = (match[1].str() == "-") ? -1 : 1 ;
        int hours = std::stoi(match[2].str()) ;
        int minutes = match[3].matched ? std::stoi(match[3].str()) : 0 ;
        offset_seconds = sign * (hours * 3600 + minutes * 60) ;
        return true ;
    }

    // Try IANA timezone format: Continent/City, Region/Timezone
    std::regex iana_re(R"(^[A-Za-z_]+/[A-Za-z0-9_]+(?:/[A-Za-z0-9_]+)?$)") ;
    if ( std::regex_match(tz, iana_re) ) {
        // Save original TZ
        const char *original_tz = std::getenv("TZ") ;
        std::string saved_tz = original_tz ? original_tz : "" ;

        // Set TZ to the IANA timezone and recalculate
        if ( setenv("TZ", tz.c_str(), 1) == 0 ) {
            tzset() ;

            // Get current time and compute offset
            std::time_t now = std::time(nullptr) ;
            std::tm *tm_local = std::localtime(&now) ;
            std::tm *tm_utc = std::gmtime(&now) ;

            if ( tm_local && tm_utc ) {
                // Calculate offset in seconds
                std::time_t local_t = std::mktime(tm_local) ;
                // Note: mktime converts to UTC, so we need the raw offset
                offset_seconds = static_cast<int>(tm_local->tm_gmtoff) ;

                // Restore original TZ
                if ( saved_tz.empty() ) {
                    unsetenv("TZ") ;
                } else {
                    setenv("TZ", saved_tz.c_str(), 1) ;
                }
                tzset() ;

                return true ;
            }

            // Restore original TZ on error
            if ( saved_tz.empty() ) {
                unsetenv("TZ") ;
            } else {
                setenv("TZ", saved_tz.c_str(), 1) ;
            }
            tzset() ;
        }
    }

    return false ;
}

int64_t strtotime(const std::string &src, const std::string &tz) {
    std::string lower_src = to_lower_copy(src) ;
    if ( lower_src.empty() || lower_src == "now" )
        return static_cast<int64_t>(std::time(nullptr)) ;

   
    if ( lower_src == "today" ) {
        auto now = std::time(nullptr) ;
        std::tm tm ;
        if ( tz.empty() || !is_iana_timezone(tz) ) {
            int offset_seconds = 0 ;
            if ( !tz.empty() && parse_timezone_offset(tz, offset_seconds) && !is_iana_timezone(tz) ) {
                std::time_t adjusted = now + offset_seconds ;
                tm = *std::gmtime(&adjusted) ;
            } else {
                tm = *std::localtime(&now) ;
            }
        } else {
            const char *original_tz = std::getenv("TZ") ;
            std::string saved_tz = original_tz ? original_tz : "" ;
            setenv("TZ", tz.c_str(), 1) ;
            tzset() ;
            tm = *std::localtime(&now) ;
            if ( saved_tz.empty() )
                unsetenv("TZ") ;
            else
                setenv("TZ", saved_tz.c_str(), 1) ;
            tzset() ;
        }
        tm.tm_hour = 0 ;
        tm.tm_min = 0 ;
        tm.tm_sec = 0 ;
        int64_t result = mktime_with_tz(tm, tz) ;
        if ( result < 0 )
            throw std::runtime_error("Failed to parse date string: " + src) ;
        return result ;
    }

    if ( lower_src == "tomorrow" ) {
        auto now = std::time(nullptr) ;
        std::tm tm ;
        if ( tz.empty() || !is_iana_timezone(tz) ) {
            int offset_seconds = 0 ;
            if ( !tz.empty() && parse_timezone_offset(tz, offset_seconds) && !is_iana_timezone(tz) ) {
                std::time_t adjusted = now + offset_seconds ;
                tm = *std::gmtime(&adjusted) ;
            } else {
                tm = *std::localtime(&now) ;
            }
        } else {
            const char *original_tz = std::getenv("TZ") ;
            std::string saved_tz = original_tz ? original_tz : "" ;
            setenv("TZ", tz.c_str(), 1) ;
            tzset() ;
            tm = *std::localtime(&now) ;
            if ( saved_tz.empty() )
                unsetenv("TZ") ;
            else
                setenv("TZ", saved_tz.c_str(), 1) ;
            tzset() ;
        }
        tm.tm_mday += 1 ;
        tm.tm_hour = 0 ;
        tm.tm_min = 0 ;
        tm.tm_sec = 0 ;
        int64_t result = mktime_with_tz(tm, tz) ;
        if ( result < 0 )
            throw std::runtime_error("Failed to parse date string: " + src) ;
        return result ;
    }

    if ( lower_src == "yesterday" ) {
        auto now = std::time(nullptr) ;
        std::tm tm ;
        if ( tz.empty() || !is_iana_timezone(tz) ) {
            int offset_seconds = 0 ;
            if ( !tz.empty() && parse_timezone_offset(tz, offset_seconds) && !is_iana_timezone(tz) ) {
                std::time_t adjusted = now + offset_seconds ;
                tm = *std::gmtime(&adjusted) ;
            } else {
                tm = *std::localtime(&now) ;
            }
        } else {
            const char *original_tz = std::getenv("TZ") ;
            std::string saved_tz = original_tz ? original_tz : "" ;
            setenv("TZ", tz.c_str(), 1) ;
            tzset() ;
            tm = *std::localtime(&now) ;
            if ( saved_tz.empty() )
                unsetenv("TZ") ;
            else
                setenv("TZ", saved_tz.c_str(), 1) ;
            tzset() ;
        }
        tm.tm_mday -= 1 ;
        tm.tm_hour = 0 ;
        tm.tm_min = 0 ;
        tm.tm_sec = 0 ;
        int64_t result = mktime_with_tz(tm, tz) ;
        if ( result < 0 )
            throw std::runtime_error("Failed to parse date string: " + src) ;
        return result ;
    }

    bool all_digits = !src.empty() && (std::all_of(src.begin(), src.end(), ::isdigit) || (src[0] == '-' && src.size() > 1 && std::all_of(src.begin() + 1, src.end(), ::isdigit)));
    if ( all_digits ) {
        try {
            return std::stoll(src) ;
        } catch (...) {
        }
    }

    std::tm tm = {} ;
    if ( parse_fixed_date_formats(src, tm) ) {
        int64_t result = mktime_with_tz(tm, tz) ;
        if ( result >= 0 )
            return result ;
    }

    std::regex rel_re(R"(^([+-]?\d+)\s*(second|minute|hour|day|week|month|year)s?$)", std::regex::icase) ;
    std::smatch match ;
    if ( std::regex_match(lower_src, match, rel_re) ) {
        int64_t base_s = static_cast<int64_t>(std::time(nullptr)) ;
        int64_t value = std::stoll(match[1].str()) ;
        if ( apply_relative_offset(base_s, value, match[2].str()) )
            return base_s ;
    }

    std::istringstream in(src) ;
    in >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S") ;
    if ( !in.fail() ) {
        tm.tm_isdst = -1 ;
        return static_cast<int64_t>(std::mktime(&tm)) ;
    }

    throw std::runtime_error("Failed to parse date string: " + src) ;
}

}
