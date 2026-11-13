#pragma once

#include "core/engine_config.hpp"

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace WinOpty::Core {

// Parser JSON/JSONC ligero y sin dependencias externas.
// Soporta comentarios // y /* ... */ preservando cadenas literales.
struct JsonValue {
    using Array  = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue>;
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;

    Storage data = nullptr;

    bool IsNull() const   { return std::holds_alternative<std::nullptr_t>(data); }
    bool IsBool() const   { return std::holds_alternative<bool>(data); }
    bool IsNumber() const { return std::holds_alternative<double>(data); }
    bool IsString() const { return std::holds_alternative<std::string>(data); }
    bool IsArray() const  { return std::holds_alternative<Array>(data); }
    bool IsObject() const { return std::holds_alternative<Object>(data); }

    bool AsBool(bool fallback = false) const {
        return IsBool() ? std::get<bool>(data) : fallback;
    }

    double AsDouble(double fallback = 0.0) const {
        return IsNumber() ? std::get<double>(data) : fallback;
    }

    uint64_t AsUInt64(uint64_t fallback = 0) const {
        if (!IsNumber()) return fallback;
        double v = std::get<double>(data);
        return v < 0.0 ? fallback : static_cast<uint64_t>(v);
    }

    std::string AsString(const std::string& fallback = {}) const {
        return IsString() ? std::get<std::string>(data) : fallback;
    }

    const Array& AsArray() const {
        static const Array empty;
        return IsArray() ? std::get<Array>(data) : empty;
    }

    const Object& AsObject() const {
        static const Object empty;
        return IsObject() ? std::get<Object>(data) : empty;
    }

    const JsonValue* Get(const std::string& key) const {
        if (!IsObject()) return nullptr;
        const auto& obj = std::get<Object>(data);
        auto it = obj.find(key);
        return it == obj.end() ? nullptr : &it->second;
    }

    static std::string StripJsonComments(const std::string& input) {
        std::string out;
        out.reserve(input.size());

        bool inString = false;
        bool inLineComment = false;
        bool inBlockComment = false;

        for (size_t i = 0; i < input.size(); ++i) {
            char c = input[i];
            char next = (i + 1 < input.size()) ? input[i + 1] : '\0';

            if (inLineComment) {
                if (c == '\n') {
                    inLineComment = false;
                    out.push_back(c);
                }
                continue;
            }

            if (inBlockComment) {
                if (c == '*' && next == '/') {
                    inBlockComment = false;
                    ++i;
                } else if (c == '\n') {
                    out.push_back('\n');
                }
                continue;
            }

            if (inString) {
                out.push_back(c);
                if (c == '\\' && i + 1 < input.size()) {
                    out.push_back(input[++i]);
                } else if (c == '"') {
                    inString = false;
                }
                continue;
            }

            if (c == '"') {
                inString = true;
                out.push_back(c);
            } else if (c == '/' && next == '/') {
                inLineComment = true;
                ++i;
            } else if (c == '/' && next == '*') {
                inBlockComment = true;
                ++i;
            } else {
                out.push_back(c);
            }
        }

        return out;
    }

    static Result Parse(const std::string& jsoncText, JsonValue& outValue) {
        std::string clean = StripJsonComments(jsoncText);
        Parser parser(clean);
        Result res = parser.ParseRoot(outValue);
        return res;
    }

    static Result ParseFile(const std::string& filePath, JsonValue& outValue) {
        std::ifstream ifs(filePath, std::ios::binary);
        if (!ifs) {
            return Result::Fail(StatusCode::IoError, "No se pudo abrir archivo JSONC: " + filePath);
        }
        std::ostringstream oss;
        oss << ifs.rdbuf();
        return Parse(oss.str(), outValue);
    }

    static std::string EscapeString(const std::string& s) {
        std::string out = "\"";
        for (unsigned char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (c < 0x20) {
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                        out += buf;
                    } else {
                        out.push_back(static_cast<char>(c));
                    }
            }
        }
        out.push_back('"');
        return out;
    }

private:
    class Parser {
    public:
        explicit Parser(const std::string& src) : src_(src) {}

        Result ParseRoot(JsonValue& out) {
            SkipWs();
            if (pos_ >= src_.size()) {
                return Result::Fail(StatusCode::ParseError, "JSONC vacio");
            }
            Result r = ParseValue(out);
            if (!r.ok) return r;
            SkipWs();
            if (pos_ != src_.size()) {
                return FailAt("Contenido inesperado tras la raiz JSON");
            }
            return Result::Success();
        }

    private:
        const std::string& src_;
        size_t pos_ = 0;

        void SkipWs() {
            while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) {
                ++pos_;
            }
        }

        Result FailAt(const std::string& msg) const {
            return Result::Fail(StatusCode::ParseError, msg + " (offset " + std::to_string(pos_) + ")");
        }

        Result ParseValue(JsonValue& out) {
            SkipWs();
            if (pos_ >= src_.size()) return FailAt("Fin de entrada inesperado");

            char c = src_[pos_];
            if (c == '{') return ParseObject(out);
            if (c == '[') return ParseArray(out);
            if (c == '"') {
                std::string s;
                Result r = ParseString(s);
                if (r.ok) out.data = std::move(s);
                return r;
            }
            if (c == 't' && src_.compare(pos_, 4, "true") == 0) {
                pos_ += 4;
                out.data = true;
                return Result::Success();
            }
            if (c == 'f' && src_.compare(pos_, 5, "false") == 0) {
                pos_ += 5;
                out.data = false;
                return Result::Success();
            }
            if (c == 'n' && src_.compare(pos_, 4, "null") == 0) {
                pos_ += 4;
                out.data = nullptr;
                return Result::Success();
            }
            if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
                return ParseNumber(out);
            }
            return FailAt("Token JSON inesperado");
        }

        Result ParseString(std::string& out) {
            if (src_[pos_] != '"') return FailAt("Se esperaba comilla");
            ++pos_;
            out.clear();
            while (pos_ < src_.size()) {
                char c = src_[pos_++];
                if (c == '"') return Result::Success();
                if (c == '\\') {
                    if (pos_ >= src_.size()) return FailAt("Escape incompleto");
                    char esc = src_[pos_++];
                    switch (esc) {
                        case '"':  out.push_back('"'); break;
                        case '\\': out.push_back('\\'); break;
                        case '/':  out.push_back('/'); break;
                        case 'b':  out.push_back('\b'); break;
                        case 'f':  out.push_back('\f'); break;
                        case 'n':  out.push_back('\n'); break;
                        case 'r':  out.push_back('\r'); break;
                        case 't':  out.push_back('\t'); break;
                        case 'u':
                            if (pos_ + 4 > src_.size()) return FailAt("Escape \\u invalido");
                            pos_ += 4;
                            out.push_back('?');
                            break;
                        default:
                            return FailAt("Caracter de escape desconocido");
                    }
                } else {
                    out.push_back(c);
                }
            }
            return FailAt("Cadena sin cerrar");
        }

        Result ParseNumber(JsonValue& out) {
            const char* start = src_.c_str() + pos_;
            char* end = nullptr;
            double val = std::strtod(start, &end);
            if (end == start) return FailAt("Numero invalido");
            pos_ = static_cast<size_t>(end - src_.c_str());
            out.data = val;
            return Result::Success();
        }

        Result ParseArray(JsonValue& out) {
            ++pos_; // '['
            Array arr;
            SkipWs();
            if (pos_ < src_.size() && src_[pos_] == ']') {
                ++pos_;
                out.data = std::move(arr);
                return Result::Success();
            }
            while (pos_ < src_.size()) {
                JsonValue elem;
                Result r = ParseValue(elem);
                if (!r.ok) return r;
                arr.push_back(std::move(elem));
                SkipWs();
                if (pos_ < src_.size() && src_[pos_] == ',') {
                    ++pos_;
                    continue;
                }
                if (pos_ < src_.size() && src_[pos_] == ']') {
                    ++pos_;
                    out.data = std::move(arr);
                    return Result::Success();
                }
                return FailAt("Se esperaba ',' o ']' en arreglo");
            }
            return FailAt("Arreglo sin cerrar");
        }

        Result ParseObject(JsonValue& out) {
            ++pos_; // '{'
            Object obj;
            SkipWs();
            if (pos_ < src_.size() && src_[pos_] == '}') {
                ++pos_;
                out.data = std::move(obj);
                return Result::Success();
            }
            while (pos_ < src_.size()) {
                SkipWs();
                std::string key;
                Result kr = ParseString(key);
                if (!kr.ok) return kr;
                SkipWs();
                if (pos_ >= src_.size() || src_[pos_] != ':') {
                    return FailAt("Se esperaba ':' tras clave");
                }
                ++pos_;
                JsonValue val;
                Result vr = ParseValue(val);
                if (!vr.ok) return vr;
                obj[std::move(key)] = std::move(val);
                SkipWs();
                if (pos_ < src_.size() && src_[pos_] == ',') {
                    ++pos_;
                    continue;
                }
                if (pos_ < src_.size() && src_[pos_] == '}') {
                    ++pos_;
                    out.data = std::move(obj);
                    return Result::Success();
                }
                return FailAt("Se esperaba ',' o '}' en objeto");
            }
            return FailAt("Objeto sin cerrar");
        }
    };
};

} // namespace WinOpty::Core
