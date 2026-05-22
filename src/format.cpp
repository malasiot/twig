#include <iostream>
#include <sstream>
#include <string>
#include <variant>
#include <vector>
#include <stdexcept>
#include <cstdio>
#include <cstring>

#include <twig/variant.hpp>

static std::string extractSpecifier(const std::string& fmt, size_t& i) {
    std::string spec = "%";
    ++i; // skip '%'

    // Flags: -, +, space, 0, #
    while (i < fmt.size() && std::strchr("-+ #0", fmt[i]))
        spec += fmt[i++];

    // Width (digits or *)
    while (i < fmt.size() && std::isdigit(fmt[i]))
        spec += fmt[i++];

    // Precision
    if (i < fmt.size() && fmt[i] == '.') {
        spec += fmt[i++];
        while (i < fmt.size() && std::isdigit(fmt[i]))
            spec += fmt[i++];
    }

    // Length modifier (h, l, ll, L)
    if (i < fmt.size() && std::strchr("hlL", fmt[i])) {
        spec += fmt[i++];
        if (i < fmt.size() && fmt[i] == 'l') // ll
            spec += fmt[i++];
    }

    // Conversion specifier (d, f, s, x, …)
    if (i < fmt.size())
        spec += fmt[i++];
    else
        throw std::runtime_error("Incomplete format specifier: " + spec);

    return spec;
}

// Apply a printf specifier to a Variant using snprintf, return the result.
static std::string applySpecifier(const std::string& spec, const twig::Variant& var) {
    char conversion = spec.back();
    char buf[256];

    switch (conversion) {
        // Integer family
        case 'd': case 'i': {
            // Rewrite specifier to use 'l' so long is passed safely
            std::string lspec = spec.substr(0, spec.size() - 1) + "ld";
            std::snprintf(buf, sizeof(buf), lspec.c_str(), var.toInteger());
            break;
        }
        case 'u': {
            std::string lspec = spec.substr(0, spec.size() - 1) + "lu";
            std::snprintf(buf, sizeof(buf), lspec.c_str(), static_cast<unsigned long>(var.toInteger()));
            break;
        }
        case 'o': {
            std::string lspec = spec.substr(0, spec.size() - 1) + "lo";
            std::snprintf(buf, sizeof(buf), lspec.c_str(), static_cast<unsigned long>(var.toInteger()));
            break;
        }
        case 'x': {
            std::string lspec = spec.substr(0, spec.size() - 1) + "lx";
            std::snprintf(buf, sizeof(buf), lspec.c_str(), static_cast<unsigned long>(var.toInteger()));
            break;
        }
        case 'X': {
            std::string lspec = spec.substr(0, spec.size() - 1) + "lX";
            std::snprintf(buf, sizeof(buf), lspec.c_str(), static_cast<unsigned long>(var.toInteger()));
            break;
        }
        // Float family
        case 'f': case 'F':
        case 'e': case 'E':
        case 'g': case 'G': {
            std::snprintf(buf, sizeof(buf), spec.c_str(), var.toFloat());
            break;
        }
        // String
        case 's': {
            std::string s = var.toString();
            std::snprintf(buf, sizeof(buf), spec.c_str(), s.c_str());
            break;
        }
        // Character
        case 'c': {
            std::snprintf(buf, sizeof(buf), spec.c_str(), static_cast<int>(var.toInteger()));
            break;
        }
        default:
            throw std::runtime_error(std::string("Unsupported specifier: ") + conversion);
    }

    return std::string(buf);
}

std::string format(const std::string& fmt, const std::vector<twig::Variant>& args) {
    std::ostringstream oss;
    size_t argIdx = 0;

    for (size_t i = 0; i < fmt.size(); ) {
        if (fmt[i] == '%') {
            // Escaped '%%' → literal '%'
            if (i + 1 < fmt.size() && fmt[i + 1] == '%') {
                oss << '%';
                i += 2;
                continue;
            }
            if (argIdx >= args.size())
                throw std::runtime_error("Too few arguments for the given placeholders");

            size_t start = i;
            std::string spec = extractSpecifier(fmt, i); // advances i
            oss << applySpecifier(spec, args[argIdx++]);
        } else {
            oss << fmt[i++];
        }
    }

    if (argIdx < args.size())
        throw std::runtime_error("Too many arguments for the given placeholders");

    return oss.str();
}