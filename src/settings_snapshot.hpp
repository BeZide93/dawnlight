#pragma once

#include <charconv>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <variant>

namespace dawnlight::settings {
using Value = std::variant<bool, int64_t, std::string>;
using Values = std::map<std::string, Value>;
inline constexpr size_t MaxBytes = 1024 * 1024;

// This versioned format contains only registered bool/int/string settings.
// Do not reuse the permissive legacy HUD field reader for whole-config imports.
class Reader {
    std::string_view text;
    size_t pos = 0;
    void whitespace() { while (pos < text.size() && (text[pos]==' ' || text[pos]=='\n' || text[pos]=='\r' || text[pos]=='\t')) ++pos; }
    bool take(char c) { whitespace(); if (pos == text.size() || text[pos] != c) return false; ++pos; return true; }
    bool hex(uint32_t& out) {
        out = 0;
        for (int i=0;i<4;++i) {
            if (pos == text.size()) return false;
            char c=text[pos++]; unsigned n;
            if (c>='0' && c<='9') n=c-'0';
            else if (c>='a' && c<='f') n=c-'a'+10;
            else if (c>='A' && c<='F') n=c-'A'+10;
            else return false;
            out = (out << 4) | n;
        }
        return true;
    }
    static void utf8(std::string& out, uint32_t c) {
        if (c < 0x80) out += char(c);
        else if (c < 0x800) { out += char(0xc0 | (c >> 6)); out += char(0x80 | (c & 63)); }
        else if (c < 0x10000) { out += char(0xe0 | (c >> 12)); out += char(0x80 | ((c >> 6) & 63)); out += char(0x80 | (c & 63)); }
        else { out += char(0xf0 | (c >> 18)); out += char(0x80 | ((c >> 12) & 63)); out += char(0x80 | ((c >> 6) & 63)); out += char(0x80 | (c & 63)); }
    }
    bool string(std::string& out) {
        if (!take('"')) return false;
        out.clear();
        while (pos < text.size()) {
            unsigned char c=text[pos++];
            if (c=='"') return true;
            if (c<0x20) return false;
            if (c!='\\') { out += char(c); continue; }
            if (pos==text.size()) return false;
            switch (text[pos++]) {
            case '"': out+='"'; break;
            case '\\': out+='\\'; break;
            case '/': out+='/'; break;
            case 'b': out+='\b'; break;
            case 'f': out+='\f'; break;
            case 'n': out+='\n'; break;
            case 'r': out+='\r'; break;
            case 't': out+='\t'; break;
            case 'u': {
                uint32_t code=0;
                if (!hex(code) || (code>=0xdc00 && code<=0xdfff)) return false;
                if (code>=0xd800 && code<=0xdbff) {
                    if (text.substr(pos,2)!="\\u") return false;
                    pos+=2;uint32_t low=0;
                    if (!hex(low) || low<0xdc00 || low>0xdfff) return false;
                    code=0x10000+((code-0xd800)<<10)+(low-0xdc00);
                }
                if (code==0) return false; // ConfigService strings are NUL terminated.
                utf8(out,code);break;
            }
            default: return false;
            }
        }
        return false;
    }
    bool value(Value& out) {
        whitespace(); if (pos==text.size()) return false;
        if (text[pos]=='"') { std::string s; if (!string(s)) return false; out=std::move(s);return true; }
        for (auto literal : {std::string_view("true"), std::string_view("false")}) {
            if (text.substr(pos,literal.size())==literal) {pos+=literal.size();out=literal=="true";return true;}
        }
        const size_t start=pos;
        if (text[pos]=='-') ++pos;
        if (pos==text.size() || text[pos]<'0' || text[pos]>'9') return false;
        if (text[pos]=='0') ++pos;
        else while (pos<text.size() && text[pos]>='0' && text[pos]<='9') ++pos;
        int64_t number=0;
        if (std::from_chars(text.data()+start,text.data()+pos,number).ec!=std::errc{}) return false;
        out=number; return true;
    }
    template<class Field> bool object(Field field) {
        if (!take('{')) return false;
        if (take('}')) return true;
        do {
            std::string key;
            if (!string(key) || !take(':') || !field(key)) return false;
            if (take('}')) return true;
        } while (take(','));
        return false;
    }
public:
    explicit Reader(std::string_view input) : text(input) {}
    bool decode(Values& values) {
        if (text.size()>MaxBytes) return false;
        // Accept a UTF-8 BOM from text editors.
        if (text.substr(0,3)=="\xef\xbb\xbf") pos=3;
        bool format=false, version=false, settings=false;
        Values parsed;
        if (!object([&](const std::string& key) {
            if (key=="settings") {
                if (settings) return false;
                settings=true;
                return object([&](const std::string& name) {
                    Value v;
                    return value(v) && parsed.emplace(name,std::move(v)).second;
                });
            }
            Value v;
            if (!value(v)) return false;
            if (key=="format" && !format) {
                format=std::holds_alternative<std::string>(v) && std::get<std::string>(v)=="dawnlight-settings";
                return format;
            }
            if (key=="version" && !version) {
                version=std::holds_alternative<int64_t>(v) && std::get<int64_t>(v)==1;
                return version;
            }
            return false;
        })) return false;
        whitespace();
        if (pos!=text.size() || !format || !version || !settings || parsed.empty()) return false;
        values=std::move(parsed);return true;
    }
};

inline std::string quote(std::string_view text) {
    constexpr char hex[]="0123456789abcdef";
    std::string out="\"";
    for (unsigned char c : text) {
        if (c=='"' || c=='\\') {out+='\\';out+=char(c);}
        else if (c<0x20) {out+="\\u00";out+=hex[c>>4];out+=hex[c&15];}
        else out+=char(c);
    }
    return out+'"';
}

inline std::string encode(const Values& values) {
    std::string out="{\n  \"format\": \"dawnlight-settings\",\n  \"version\": 1,\n  \"settings\": {\n";
    bool first=true;
    for (const auto& [key,value] : values) {
        if (!first) out+=",\n";
        first=false;out+="    "+quote(key)+": ";
        if (auto* b=std::get_if<bool>(&value)) out+=*b?"true":"false";
        else if (auto* i=std::get_if<int64_t>(&value)) out+=std::to_string(*i);
        else out+=quote(std::get<std::string>(value));
    }
    return out+"\n  }\n}\n";
}
} // namespace dawnlight::settings
