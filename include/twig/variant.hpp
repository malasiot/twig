#ifndef TWIG_VARIANT_HPP
#define TWIG_VARIANT_HPP

#include <cstdint>
#include <map>
#include <vector>
#include <iomanip>
#include <iostream>
#include <functional>
#include <sstream>
#include <string>
#include <cassert>
#include <chrono>

// very lighweight write-only variant class e.g. to enable json responses and passed to template engines
//
// e.g.  Variant v(Variant::Object{
//                              {"name", 3},
//                              {"values", Variant::Array{ {2,  "s" } } }
//                }) ;
//       cout << v.toJSON() << endl ;
namespace twig {


class TypeConversionException: public std::runtime_error {
public:

    TypeConversionException(const std::string &msg): std::runtime_error(msg) {
    }

};

class Variant {

public:

    using Object = std::map<std::string, Variant> ;
    using Array = std::vector<Variant> ;
    using Function = std::function<Variant(const Variant &)> ;

    using signed_integer_t = int64_t ;
    using unsigned_integer_t = uint64_t ;
    using float_t = double ;
    using string_t = std::string ;
    using boolean_t = bool ;

    using Dictionary = std::map<std::string, std::string> ;

    enum class Type : uint8_t {
        Undefined, Null,  Object, Array, String, SafeString, Boolean, Integer, Float, Function, DateTime, Duration
    };

    // constructors

    Variant(): tag_(Type::Undefined) {}

    Variant(boolean_t v) noexcept : tag_(Type::Boolean) { data_.b_ = v ; }

    Variant(int v) noexcept: Variant((int64_t)v) {}
    Variant(unsigned int v) noexcept: Variant((int64_t)v) {}

    Variant(int64_t v) noexcept: tag_(Type::Integer) { data_.i_ = v ; }
    Variant(uint64_t v) noexcept: tag_(Type::Integer) { data_.i_ = (int64_t)v ; }

    Variant(Function v): tag_(Type::Function) { new (&data_.fp_) Function(v) ; }

    Variant(float_t v) noexcept: tag_(Type::Float) { data_.f_ = v ; }

    Variant(const char *value, bool safe = false) {
        tag_ = ( safe ) ? Type::SafeString : Type::String ;
        new (&data_.s_) string_t(value) ;
    }

    Variant(const string_t& value, bool safe = false) {
        tag_ = ( safe ) ? Type::SafeString : Type::String ;
        new (&data_.s_) string_t(value) ;
    }

    Variant(string_t&& value, bool safe = false)  {
        tag_ = ( safe ) ? Type::SafeString : Type::String ;
        new (&data_.s_) string_t(value) ;
    }


    Variant(const Object& value): tag_(Type::Object) {
        new (&data_.o_) Object(value) ;
    }

    Variant(Object&& value): tag_(Type::Object) {
        new (&data_.o_) Object(std::move(value)) ;
    }

    Variant(const Array& value): tag_(Type::Array) {
        new (&data_.a_) Array(value) ;
    }

    Variant(Array&& value): tag_(Type::Array) {
        new (&data_.a_) Array(std::move(value)) ;
    }

    // DateTime constructor from std::chrono::system_clock::time_point
    Variant(const std::chrono::system_clock::time_point &tp): tag_(Type::DateTime) {
        auto duration = tp.time_since_epoch() ;
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(duration) ;
        data_.ts_ = secs.count() ;
    }

    // Duration constructor from std::chrono::duration
    template<typename Rep, typename Period>
    Variant(const std::chrono::duration<Rep, Period> &dur): tag_(Type::Duration) {
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(dur) ;
        data_.ts_ = secs.count() ;
    }

    ~Variant() {
        destroy() ;
    }

    Variant(const Variant& other) {
        create(other) ;
    }

    Variant &operator=(const Variant &other) {
        if ( this != &other ) {
            destroy() ;
            create(other) ;
        }
        return *this ;
    }

    Variant(Variant&& other): tag_(other.tag_) {
        switch (tag_)
        {
        case Type::Object:
            new (&data_.o_) Object(std::move(other.data_.o_)) ;
            break;
        case Type::Array:
            new (&data_.a_) Array(std::move(other.data_.a_)) ;
            break;
        case Type::String:
        case Type::SafeString:
            new (&data_.s_) string_t(std::move(other.data_.s_)) ;
            break;
        case Type::Boolean:
            data_.b_ = other.data_.b_ ;
            break;
        case Type::Integer:
            data_.i_ = other.data_.i_ ;
            break;
        case Type::Float:
            data_.f_ = other.data_.f_ ;
            break;
        case Type::Function:
            new (&data_.fp_) Function(std::move(other.data_.fp_)) ;
            break;
        case Type::DateTime:
        case Type::Duration:
            data_.ts_ = other.data_.ts_ ;
            break;
        default:
            break;
        }

        other.tag_ = Type::Undefined ;
    }

    // make Object from a dictionary
    static Variant fromDictionary(const Dictionary &dict) {
        Variant::Object obj ;
        for( const auto &p: dict )
            obj.insert({p.first, p.second}) ;
        return obj ;
    }

    // make Array from dictionary where each element is the Object {<keyname>: <key>, <valname>: <val>}
    static Variant fromDictionaryAsArray(const Dictionary &dict, const std::string &keyname = "key", const std::string &valname = "val") {
        Variant::Array ar ;
        for( const auto &p: dict )
            ar.emplace_back(Variant::Object({{keyname, p.first}, {valname, p.second}})) ;
        return ar ;
    }

    // make Array from a vector of values
    template<class T>
    static Variant fromVector(const std::vector<T> &vals) {
        Variant::Array ar ;
        for( const auto &p: vals )
            ar.push_back(p) ;
        return ar ;
    }

    void append(const std::string &key, const Variant &val) {
        if ( isObject() )
            data_.o_.insert({key, val}) ;
    }

    void append(const Variant &val) {
        if ( isArray() )
            data_.a_.push_back(val) ;
    }

    // check object type

    bool isObject() const { return tag_ == Type::Object ; }
    bool isArray() const { return tag_ == Type::Array ; }
    bool isBoolean() const { return tag_ == Type::Boolean ; }
    bool isInteger() const { return tag_ == Type::Integer ; }
    bool isNull() const { return tag_ == Type::Null ; }
    bool isUndefined() const { return tag_ == Type::Undefined ; }
    bool isSafe() const { return ( tag_ != Type::String ) ; }

    bool isString() const { return  (tag_ == Type::String) || (tag_ == Type::SafeString ); }
    bool isNumber() const {
        return ( tag_ == Type::Integer ) ||
                ( tag_ == Type::Float ) ||
                ( tag_ == Type::Boolean );
    }

    bool isDateTime() const { return tag_ == Type::DateTime ; }
    bool isDuration() const { return tag_ == Type::Duration ; }

    // check if variant stores simple type string, number, integer or boolean
    bool isPrimitive() const {
        return ( tag_ == Type::String ||
                 tag_ == Type::SafeString ||
                 tag_ == Type::Integer ||
                 tag_ == Type::Float ||
                 tag_ == Type::Boolean
                 ) ;
    }

    // False are booleans with false values and empty arrays
    bool isFalse() const {
        if ( tag_ == Type::Boolean ) return !data_.b_ ;
        else if ( tag_ == Type::Array ) return data_.a_.size() == 0 ;
        else if ( tag_ == Type::Null ) return true ;
        else return false ;
    }

    // convert value to string
    std::string toString() const {
        switch (tag_)
        {
        case Type::String:
        case Type::SafeString:
            return data_.s_;
        case Type::Boolean: {
            std::ostringstream strm ;
            strm << data_.b_ ;
            return strm.str() ;
        }
        case Type::Integer:
            return std::to_string(data_.i_) ;
        case Type::Float: {
            char buf[64];
            // Use general format with sufficient precision and no unnecessary trailing zeros
            int n = snprintf(buf, sizeof(buf), "%.15g", data_.f_);
            if (n > 0) return std::string(buf);
            return std::to_string(data_.f_);
        }
        case Type::DateTime: {
            auto tp = std::chrono::system_clock::time_point(std::chrono::seconds(data_.ts_)) ;
            auto time = std::chrono::system_clock::to_time_t(tp) ;
            std::ostringstream strm ;
            strm << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ") ;
            return strm.str() ;
        }
        case Type::Duration: {
            int64_t secs = data_.ts_ ;
            std::ostringstream strm ;

            if (secs < 60) {
                strm << secs << "s" ;
            } else if (secs < 3600) {
                strm << (secs / 60) << "m" ;
            } else {
                strm << (secs / 3600) << "h" ;
            }
            return strm.str() ;
        }
        default:
            return std::string();
        }
    }

    double toFloat() const {
        switch (tag_)
        {
        case Type::String:
        case Type::SafeString:
            try {
            return std::stod(data_.s_);
        }
            catch ( ... ) {
            return 0.0 ;
        }

        case Type::Boolean:
            return (double)data_.b_ ;
        case Type::Integer:
            return (double)data_.i_ ;
        case Type::Float:
            return (double)data_.f_ ;
        default:
            return 0.0;
        }
    }

    int64_t toInteger() const {
        switch (tag_)
        {
        case Type::String:
        case Type::SafeString:
            try {
            return std::stoi(data_.s_);
        }
            catch ( ... ) {
            return 0 ;
        }

        case Type::Boolean:
            return (int64_t)data_.b_ ;
        case Type::Integer:
            return (int64_t)data_.i_ ;
        case Type::Float:
            return (int64_t)data_.f_ ;
        case Type::DateTime:
        case Type::Duration:
            return (int64_t)data_.ts_ ;
        default:
            return 0;
        }
    }

    Variant toNumber() const {
        switch (tag_)
        {
        case Type::String:
        case Type::SafeString:
        try {
            return static_cast<int64_t>(std::stoll(data_.s_)) ;
        }
        catch ( ... ) {
            try {
                return static_cast<double>(std::stoll(data_.s_)) ;
            }
            catch ( ... ) {
                throw TypeConversionException("Cannot convert string to number: " + data_.s_) ;
            }
        }

        case Type::Boolean:
            return static_cast<int64_t>(data_.b_) ;
        case Type::Integer:
            return data_.i_ ;
        case Type::Float:
            return static_cast<double>(data_.f_) ;
        case Type::DateTime:
        case Type::Duration:
            return static_cast<int64_t>(data_.ts_) ;
        default:
            return static_cast<int64_t>(0);
        }

    }

    bool toBoolean() const {
        switch (tag_)
        {
        case Type::String:
        case Type::SafeString:
            return !(data_.s_.empty()) ;
        case Type::Boolean:
            return data_.b_ ;
        case Type::Integer:
            return static_cast<bool>(data_.i_) ;
        case Type::Float:
            return static_cast<bool>(data_.f_ != 0.0f) ;
        case Type::DateTime:
        case Type::Duration:
            return data_.ts_ != 0 ;
        default:
            return false;
        }
    }

    // Get DateTime value in seconds since epoch
    int64_t toDateTime() const {
        switch (tag_)
        {
        case Type::DateTime:
        case Type::Duration:
            return data_.ts_ ;
        case Type::Integer:
            return data_.i_ ;
        default:
            return 0 ;
        }
    }

    // Get Duration value in seconds
    int64_t toDuration() const {
        switch (tag_)
        {
        case Type::DateTime:
        case Type::Duration:
            return data_.ts_ ;
        case Type::Integer:
            return data_.i_ ;
        default:
            return 0 ;
        }
    }

    Object toObject() const {
        switch (tag_)
        {
        case Type::Object:
            return data_.o_ ;

        default:
            return Object();
        }
    }

     Array toArray() const {
        switch (tag_)
        {
        case Type::Array:
            return data_.a_ ;

        default:
            return Array();
        }
    }

    // Return the keys of an Object otherwise an empty list
    std::vector<std::string> keys() const {
        std::vector<std::string> res ;

        if ( !isObject() ) return res ;

        for ( const auto &p: data_.o_ )
            res.push_back(p.first) ;
        return res ;
    }

    // length of object or array or string, zero otherwise
    size_t length() const {
        if ( isObject() )
            return data_.o_.size() ;
        else if ( isArray() ) {
            return data_.a_.size() ;
        } else if ( ( tag_ == Type::String ) || ( tag_ == Type::SafeString ) )
            return data_.s_.length() ;
        else return 0 ;
    }

    // Returns a member value given the key. The key is of the form <member1>[.<member2>. ... <memberN>]
    // If this is not an object or the key is not found it returns a Null


    const Variant &at(const std::string &key) const {

        if ( key.empty() ) return Variant::undefined() ;

        const Variant *current = this ;
        if ( !current->isObject() ) return undefined() ;

        size_t start = 0, end = 0;

        while ( end != std::string::npos) {
            end = key.find('.', start) ;
            std::string subkey = key.substr(start, end == std::string::npos ? std::string::npos : end - start) ;

            const Variant &val = current->fetchKey(subkey) ;
            if ( val.isUndefined() ) return val ;

            start = end + 1 ;

            if ( end != std::string::npos ) current = &val ;
            else return val ;
        }

        return Variant::undefined() ;
    }

    Variant get(const std::string &key, const Variant &default_val) const {
        const auto &v = (*this).at(key) ;
        return v.isUndefined() ? default_val : v ;
    }

    // return an element of an array
    const Variant &at(uint idx) const { return fetchIndex(idx) ; }

    // overloaded indexing operators
    const Variant &operator [] (const std::string &key) const {
        return fetchKey(key) ;
    }

    const Variant &operator [] (uint idx) const {
        return fetchIndex(idx) ;
    }

    Type type() const { return tag_ ; }

    // JSON encoder
    void toJSON(std::ostream &strm) const {

        switch ( tag_ ) {
        case Type::Object: {
            strm << "{" ;
            auto it = data_.o_.cbegin() ;
            if ( it != data_.o_.cend() ) {
                strm << json_escape_string(it->first) << ": " ;
                it->second.toJSON(strm) ;
                ++it ;
            }
            while ( it != data_.o_.cend() ) {
                strm << ", " ;
                strm << json_escape_string(it->first) << ": " ;
                it->second.toJSON(strm) ;
                ++it ;
            }
            strm << "}" ;
            break ;
        }
        case Type::Array: {
            strm << "[" ;
            auto it = data_.a_.cbegin() ;
            if ( it != data_.a_.cend() ) {
                it->toJSON(strm) ;
                ++it ;
            }
            while ( it != data_.a_.cend() ) {
                strm << ", " ;
                it->toJSON(strm) ;
                ++it ;
            }

            strm << "]" ;
            break ;
        }
        case Type::String:
        case Type::SafeString:
        {
            strm << json_escape_string(data_.s_) ;
            break ;
        }
        case Type::Boolean: {
            strm << ( data_.b_ ? "true" : "false") ;
            break ;
        }
        case Type::Null: {
            strm << "null" ;
            break ;
        }
        case Type::Float: {
            strm << data_.f_ ;
            break ;
        }
        case Type::Integer: {
            strm << data_.i_ ;
            break ;
        }
        case Type::DateTime: {
            // Format as ISO 8601 timestamp in seconds or convert to string
            strm << json_escape_string(toString()) ;
            break ;
        }
        case Type::Duration: {
            // Format duration as seconds
            strm << json_escape_string(toString()) ;
            break ;
        }
        }
    }

    std::string toJSON() const {
        std::ostringstream strm ;
        toJSON(strm) ;
        return strm.str() ;
    }


    template<bool Const, bool Reverse> 
    class iterator {
  
        using iterator_category = std::bidirectional_iterator_tag; 
        using value_type        = Variant;
        using difference_type   = std::ptrdiff_t;
        using pointer           = Variant*;
        using reference         = Variant&;

        using array_iter_t = std::conditional_t<Const, typename Array::const_iterator, typename Array::iterator>;
        using obj_iter_t = std::conditional_t<Const, typename Object::const_iterator, typename Object::iterator>;

        using array_base_t = std::conditional_t<Reverse, std::reverse_iterator<array_iter_t>, array_iter_t>;
        using obj_base_t = std::conditional_t<Reverse, std::reverse_iterator<obj_iter_t>, obj_iter_t>;

        using value_reference = std::conditional_t<Const, const Variant&, Variant&>;

        void destroy() {
            if ( kind_ == ArrayType )
                vec_it_.~array_base_t();
            else
                map_it_.~obj_base_t();
        }

    public:

        iterator(array_base_t it): kind_(ArrayType), vec_it_(it) {}
        iterator(obj_base_t it): kind_(ObjectType), map_it_(it) {}

        iterator(const iterator& other): kind_(other.kind_) {
            if ( kind_ == ArrayType )
                new (&vec_it_) array_base_t(other.vec_it_);
            else
                new (&map_it_) obj_base_t(other.map_it_);
        }

        ~iterator() {
            destroy();
        }

        iterator& operator=(const iterator& other) {
            if (this == &other)
                return *this;

            destroy();

            kind_ = other.kind_;

            if ( kind_ == ArrayType )
                new (&vec_it_) array_base_t(other.vec_it_);
            else
                new (&map_it_) obj_base_t(other.map_it_);

            return *this;
        }

        iterator& operator++() {
            if ( kind_ == ArrayType )
                ++vec_it_;
            else
                ++map_it_;

            return *this;
        }

        iterator& operator--() {
            if ( kind_ == ArrayType )
                --vec_it_;
            else
                --map_it_;

            return *this;
        }

        bool operator==(const iterator& other) const {
            if (kind_ != other.kind_) return false;

           if ( kind_ == ArrayType )
                return vec_it_ == other.vec_it_;
            else
                return map_it_ == other.map_it_;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }

        value_reference value() {
          if ( kind_ == ArrayType )
                return (*vec_it_);
            else
                return (map_it_->second);
        }

        const Variant &value() const {
            if ( kind_ == ArrayType )
                return (*vec_it_);
            else
                return (map_it_->second);
        }

        const std::string& key() const {
            if ( kind_ != ObjectType ) return std::string() ;
            else return map_it_->first;
        }

        value_reference operator*() {
            if ( kind_ == ArrayType )
                return (*vec_it_);
            else
                return (*map_it_).second;
        }

        const Variant &operator*() const {
            if ( kind_ == ArrayType )
                return (*vec_it_);
            else
                return (*map_it_).second;
        }


    private:

        enum kind_t { ObjectType, ArrayType } ;

        kind_t kind_;

        union {
            array_base_t vec_it_;
            obj_base_t map_it_;
        };
    } ;


    using iterator_t =
        iterator<false, false>;

    using const_iterator_t =
        iterator<true, false>;

    using reverse_iterator_t =
        iterator<false, true>;

    using const_reverse_iterator_t =
        iterator<true, true>;

    iterator_t begin() {
        if ( isArray() )
            return iterator_t(data_.a_.begin());
        else if ( isObject() )
            return iterator_t(data_.o_.begin());
        else
            throw std::logic_error("iterator cannot be applied to this object");
    }

     const_iterator_t begin() const {
        if ( isArray() )
            return const_iterator_t(data_.a_.begin());
        else if ( isObject() )
            return const_iterator_t(data_.o_.begin());
        else
            throw std::logic_error("iterator cannot be applied to this object");
    }

    iterator_t end() {
        if ( isArray() )
            return iterator_t(data_.a_.end());
        else if ( isObject() )
            return iterator_t(data_.o_.end());
        else
            throw std::logic_error("iterator cannot be applied to this object");
    }

    const_iterator_t end() const {
        if ( isArray() )
            return const_iterator_t(data_.a_.end());
        else if ( isObject() )
            return const_iterator_t(data_.o_.end());
        else
            throw std::logic_error("iterator cannot be applied to this object");
    }

    const_iterator_t cbegin() {
        if ( isArray() )
            return const_iterator_t(data_.a_.cbegin());
        else if ( isObject() )
            return const_iterator_t(data_.o_.cbegin());
        else
            throw std::logic_error("iterator cannot be applied to this object");
    }

    const const_iterator_t cend() {
        if ( isArray() )
            return const_iterator_t(data_.a_.end());
        else if ( isObject() )
            return const_iterator_t(data_.o_.end());
        else
            throw std::logic_error("iterator cannot be applied to this object");
    }

    reverse_iterator_t rbegin() {
        if ( isArray() )
            return reverse_iterator_t(data_.a_.rbegin());
        else if ( isObject() )
            return reverse_iterator_t(data_.o_.rbegin());
        else
            throw std::logic_error("iterator cannot be applied to this object");
    }

    const_reverse_iterator_t rbegin() const {
        if ( isArray() )
            return const_reverse_iterator_t(data_.a_.rbegin());
        else if ( isObject() )
            return const_reverse_iterator_t(data_.o_.rbegin());
        else
            throw std::logic_error("iterator cannot be applied to this object");
    }

    reverse_iterator_t rend() {
        if ( isArray() )
            return reverse_iterator_t(data_.a_.rend());
        else if ( isObject() )
            return reverse_iterator_t(data_.o_.rend());
        else
            throw std::logic_error("iterator cannot be applied to this object");
    }

    const_reverse_iterator_t rend() const {
        if ( isArray() )
            return const_reverse_iterator_t(data_.a_.rend());
        else if ( isObject() )
            return const_reverse_iterator_t(data_.o_.rend());
        else
            throw std::logic_error("iterator cannot be applied to this object");
    }


    static const Variant &null() {
        static Variant null_value ;
        null_value.tag_ = Type::Null ;
        return null_value ;
    }

    static const Variant &undefined() {
        static Variant undefined_value ;
        undefined_value.tag_ = Type::Undefined ;
        return undefined_value ;
    }

    // Factory methods for DateTime and Duration
    static Variant dateTime(int64_t seconds_since_epoch) {
        return Variant(std::chrono::system_clock::time_point(std::chrono::seconds(seconds_since_epoch))) ;
    }

    static Variant duration(int64_t seconds) {
        return Variant(std::chrono::seconds(seconds)) ;
    }

    // Create from current system time
    static Variant now() {
        return Variant(std::chrono::system_clock::now()) ;
    }

    Variant invoke(const Variant &args) const {
        if ( tag_ != Type::Function ) return undefined() ;
        else return (data_.fp_)(args) ;
    }


    // Parse JSON string into Variant. May optionaly throw a JSONParseException or otherwise return a Null ;
    static Variant fromJSONString(const std::string &src, bool throw_exception = false) ;
    static Variant fromJSONFile(const std::string &path, bool throw_exception = false) ;


private:


    // Original: https://gist.github.com/kevinkreiser/bee394c60c615e0acdad

    static std::string json_escape_string(const std::string &str) {
        std::stringstream strm ;
        strm << '"';

        for (const auto& c : str) {
            switch (c) {
            case '\\': strm << "\\\\"; break;
            case '"': strm << "\\\""; break;
            case '/': strm << "\\/"; break;
            case '\b': strm << "\\b"; break;
            case '\f': strm << "\\f"; break;
            case '\n': strm << "\\n"; break;
            case '\r': strm << "\\r"; break;
            case '\t': strm << "\\t"; break;
            default:
                if(c >= 0 && c < 32) {
                    //format changes for json hex
                    strm.setf(std::ios::hex, std::ios::basefield);
                    strm.setf(std::ios::uppercase);
                    strm.fill('0');
                    //output hex
                    strm << "\\u" << std::setw(4) << static_cast<int>(c);
                }
                else
                    strm << c;
                break;
            }
        }
        strm << '"';

        return strm.str() ;
    }


    const Variant &fetchKey(const std::string &key) const {
        if (!isObject() ) return undefined();

        auto it = data_.o_.find(key) ;
        if ( it == data_.o_.end() ) return undefined() ; // return undefined
        else return it->second ; // return reference
    }

    const Variant &fetchIndex(uint idx) const {
        if (!isArray()) return undefined() ;

        if ( idx < data_.a_.size() ) {
            const Variant &v = (data_.a_)[idx] ;
            return v ; // reference to item
        }
        else return undefined() ;
    }

    void destroy() {
        switch (tag_) {
        case Type::Object:
            data_.o_.~Object() ;
            break ;
        case Type::Array:
            data_.a_.~Array() ;
            break ;
        case Type::String:
        case Type::SafeString:
            data_.s_.~string_t() ;
            break ;
        case Type::Function:
            data_.fp_.~Function() ;
            break ;
        case Type::DateTime:
        case Type::Duration:
            // No cleanup needed for int64_t
            break ;
        default:
            break ;
        }
    }

    void create(const Variant &other) {
        tag_ = other.tag_ ;
        switch (tag_)
        {
        case Type::Object:
            new ( &data_.o_ ) Object(other.data_.o_) ;
            break;
        case Type::Array:
            new ( &data_.a_ ) Array(other.data_.a_) ;
            break;
        case Type::String:
        case Type::SafeString:
            new ( &data_.s_ ) string_t(other.data_.s_) ;
            break;
        case Type::Function:
            new ( &data_.fp_ ) Function(other.data_.fp_) ;
            break;
        case Type::Boolean:
            data_.b_ = other.data_.b_ ;
            break;
        case Type::Integer:
            data_.i_ = other.data_.i_ ;
            break;
        case Type::Float:
            data_.f_ = other.data_.f_ ;
            break;
        case Type::DateTime:
        case Type::Duration:
            data_.ts_ = other.data_.ts_ ;
            break;
        default:
            break;
        }
    }

private:

    union Data {
        Object    o_;
        Array     a_ ;
        string_t s_ ;
        boolean_t   b_ ;
        signed_integer_t   i_ ;
        float_t     f_ ;
        Function fp_ ;
        int64_t ts_ ;  // timestamp in seconds (for DateTime and Duration)

        Data() {}
        ~Data() {}
    } ;

    Data data_ ;
    Type tag_ ;
};


class JSONParseException: public std::runtime_error {
public:

    JSONParseException(const std::string &msg): std::runtime_error(msg) {
    }

};


}
#endif
