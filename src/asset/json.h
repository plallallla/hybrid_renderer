#pragma once

#include <cstddef>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hr
{
class JsonValue
{
public:
    enum class Type
    {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object
    };

    using Array = std::vector<JsonValue>;
    using Object = std::unordered_map<std::string, JsonValue>;

    JsonValue() = default;

    static JsonValue Null() { return JsonValue(); }
    static JsonValue Bool(bool value)
    {
        JsonValue v;
        v.type_ = Type::Bool;
        v.boolValue_ = value;
        return v;
    }
    static JsonValue Number(double value)
    {
        JsonValue v;
        v.type_ = Type::Number;
        v.numberValue_ = value;
        return v;
    }
    static JsonValue String(std::string value)
    {
        JsonValue v;
        v.type_ = Type::String;
        v.stringValue_ = std::move(value);
        return v;
    }
    static JsonValue ArrayValue(Array value)
    {
        JsonValue v;
        v.type_ = Type::Array;
        v.arrayValue_ = std::move(value);
        return v;
    }
    static JsonValue ObjectValue(Object value)
    {
        JsonValue v;
        v.type_ = Type::Object;
        v.objectValue_ = std::move(value);
        return v;
    }

    Type TypeOf() const { return type_; }
    bool IsNull() const { return type_ == Type::Null; }
    bool IsBool() const { return type_ == Type::Bool; }
    bool IsNumber() const { return type_ == Type::Number; }
    bool IsString() const { return type_ == Type::String; }
    bool IsArray() const { return type_ == Type::Array; }
    bool IsObject() const { return type_ == Type::Object; }

    bool AsBool(bool fallback = false) const { return IsBool() ? boolValue_ : fallback; }
    double AsNumber(double fallback = 0.0) const { return IsNumber() ? numberValue_ : fallback; }
    const std::string& AsString(const std::string& fallback = EmptyString()) const { return IsString() ? stringValue_ : fallback; }
    const Array& AsArray() const { return arrayValue_; }
    const Object& AsObject() const { return objectValue_; }

    const JsonValue* Find(const std::string& key) const
    {
        if (!IsObject())
        {
            return nullptr;
        }
        const auto it = objectValue_.find(key);
        return it == objectValue_.end() ? nullptr : &it->second;
    }

    const JsonValue* AtIndex(std::size_t index) const
    {
        if (!IsArray() || index >= arrayValue_.size())
        {
            return nullptr;
        }
        return &arrayValue_[index];
    }

private:
    static const std::string& EmptyString()
    {
        static const std::string empty;
        return empty;
    }

    Type type_ = Type::Null;
    bool boolValue_ = false;
    double numberValue_ = 0.0;
    std::string stringValue_;
    Array arrayValue_;
    Object objectValue_;
};

class JsonParser
{
public:
    explicit JsonParser(std::string text) : text_(std::move(text)) {}

    JsonValue Parse()
    {
        SkipWhitespace();
        JsonValue value = ParseValue();
        SkipWhitespace();
        return value;
    }

private:
    JsonValue ParseValue()
    {
        SkipWhitespace();
        if (Match("null"))
        {
            return JsonValue::Null();
        }
        if (Match("true"))
        {
            return JsonValue::Bool(true);
        }
        if (Match("false"))
        {
            return JsonValue::Bool(false);
        }

        const char c = Peek();
        if (c == '"')
        {
            return JsonValue::String(ParseString());
        }
        if (c == '{')
        {
            return ParseObject();
        }
        if (c == '[')
        {
            return ParseArray();
        }
        return JsonValue::Number(ParseNumber());
    }

    JsonValue ParseObject()
    {
        Expect('{');
        JsonValue::Object object;
        SkipWhitespace();
        if (Peek() == '}')
        {
            Advance();
            return JsonValue::ObjectValue(std::move(object));
        }

        while (true)
        {
            SkipWhitespace();
            const std::string key = ParseString();
            SkipWhitespace();
            Expect(':');
            JsonValue value = ParseValue();
            object.emplace(key, std::move(value));
            SkipWhitespace();
            if (Peek() == '}')
            {
                Advance();
                break;
            }
            Expect(',');
        }

        return JsonValue::ObjectValue(std::move(object));
    }

    JsonValue ParseArray()
    {
        Expect('[');
        JsonValue::Array array;
        SkipWhitespace();
        if (Peek() == ']')
        {
            Advance();
            return JsonValue::ArrayValue(std::move(array));
        }

        while (true)
        {
            array.push_back(ParseValue());
            SkipWhitespace();
            if (Peek() == ']')
            {
                Advance();
                break;
            }
            Expect(',');
        }

        return JsonValue::ArrayValue(std::move(array));
    }

    std::string ParseString()
    {
        Expect('"');
        std::string result;
        while (true)
        {
            const char c = Advance();
            if (c == '"')
            {
                break;
            }
            if (c == '\\')
            {
                const char escaped = Advance();
                switch (escaped)
                {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case '/': result.push_back('/'); break;
                case 'b': result.push_back('\b'); break;
                case 'f': result.push_back('\f'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                default: result.push_back(escaped); break;
                }
            }
            else
            {
                result.push_back(c);
            }
        }
        return result;
    }

    double ParseNumber()
    {
        const size_t start = pos_;
        if (Peek() == '-')
        {
            Advance();
        }
        while (std::isdigit(static_cast<unsigned char>(Peek())))
        {
            Advance();
        }
        if (Peek() == '.')
        {
            Advance();
            while (std::isdigit(static_cast<unsigned char>(Peek())))
            {
                Advance();
            }
        }
        if (Peek() == 'e' || Peek() == 'E')
        {
            Advance();
            if (Peek() == '+' || Peek() == '-')
            {
                Advance();
            }
            while (std::isdigit(static_cast<unsigned char>(Peek())))
            {
                Advance();
            }
        }

        const std::string token = text_.substr(start, pos_ - start);
        return std::strtod(token.c_str(), nullptr);
    }

    bool Match(const char* literal)
    {
        const size_t len = std::char_traits<char>::length(literal);
        if (text_.substr(pos_, len) == literal)
        {
            pos_ += len;
            return true;
        }
        return false;
    }

    void SkipWhitespace()
    {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_])))
        {
            ++pos_;
        }
    }

    char Peek() const
    {
        return pos_ < text_.size() ? text_[pos_] : '\0';
    }

    char Advance()
    {
        if (pos_ >= text_.size())
        {
            return '\0';
        }
        return text_[pos_++];
    }

    void Expect(char expected)
    {
        const char actual = Advance();
        if (actual != expected)
        {
            throw std::runtime_error("invalid json");
        }
    }

    std::string text_;
    std::size_t pos_ = 0;
};

inline bool LoadJsonFile(const std::string& path, JsonValue& out)
{
    std::ifstream file(path);
    if (!file)
    {
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    try
    {
        JsonParser parser(ss.str());
        out = parser.Parse();
        return true;
    }
    catch (...)
    {
        return false;
    }
}
} // namespace hr
