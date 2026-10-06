#include "../src/settings_snapshot.hpp"
#include <cassert>
#include <limits>

int main() {
    using namespace dawnlight::settings;
    const Values original{{"enabled",true},{"x",int64_t(-42)},
        {"min",std::numeric_limits<int64_t>::min()}, {"max",std::numeric_limits<int64_t>::max()},
        {"layout",std::string("quoted \"x\" \\ path\n\t\r\b\f UTF-8: \xc3\xa4")}};
    const auto json=encode(original);
    Values decoded;
    assert(Reader(json).decode(decoded) && decoded==original);
    assert(Reader("\xef\xbb\xbf"+json).decode(decoded));
    auto wrap=[](std::string fields){return "{\"format\":\"dawnlight-settings\",\"version\":1,\"settings\":{"+fields+"}}";};
    assert(Reader(wrap("\"layout\":\"\\u00e4\\ud83d\\ude00\"")).decode(decoded));
    assert(std::get<std::string>(decoded.at("layout"))=="\xc3\xa4\xf0\x9f\x98\x80");
    for (const char* fields : {"", "\"x\":1,", "\"x\":1,\"x\":2", "\"x\":01", "\"x\":1.5",
        "\"x\":1e2", "\"x\":9223372036854775808", "\"x\":-9223372036854775809",
        "\"x\":null", "\"x\":[]", "\"x\":{}", "\"x\":tru", "\"x\":\"\\u0000\"",
        "\"x\":\"\\ud800\"", "\"x\":\"\\udc00\"", "\"x\":\"\\q\""}) {
        const auto before=decoded;assert(!Reader(wrap(fields)).decode(decoded));assert(decoded==before);
    }
    assert(!Reader(json+"junk").decode(decoded));
    assert(!Reader(std::string(MaxBytes+1,' ')+json).decode(decoded));
    for (size_t n=0;n<json.size()-2;++n) assert(!Reader(std::string_view(json).substr(0,n)).decode(decoded));
    auto wrong=json;wrong.replace(wrong.find("\"version\": 1"),12,"\"version\": 2");
    assert(!Reader(wrong).decode(decoded));
    assert(!Reader("{\"elements\":{}}").decode(decoded));
}
