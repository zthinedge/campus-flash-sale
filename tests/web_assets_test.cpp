#include "http/HttpRequest.h"
#include "http/HttpResponse.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace
{

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    assert(input);
    std::ostringstream content;
    content << input.rdbuf();
    return content.str();
}

} // namespace

int main()
{
    const std::filesystem::path webRoot(CAMPUS_WEB_ROOT);
    const std::string html = readFile(webRoot / "index.html");
    const std::string javascript = readFile(webRoot / "app.js");
    const std::string css = readFile(webRoot / "styles.css");

    assert(html.find("Campus Flash Sale") != std::string::npos);
    assert(html.find("id=\"saleGrid\"") != std::string::npos);
    assert(javascript.find("/api/activities") != std::string::npos);
    assert(javascript.find("async function purchase") != std::string::npos);
    assert(css.find(".sale-grid") != std::string::npos);
    assert(css.find("@media") != std::string::npos);

    const std::string body = R"({"username":"student01","password":"123456"})";
    const std::string rawRequest =
        "POST /api/login?source=web HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: "
        + std::to_string(body.size()) + "\r\n\r\n" + body;

    reactor_http_kit::http::HttpRequest request;
    std::size_t consumed = 0;
    assert(reactor_http_kit::http::tryParseHttpRequest(rawRequest, request, consumed));
    assert(consumed == rawRequest.size());
    assert(request.method == "POST");
    assert(request.path == "/api/login");
    assert(request.query.at("source") == "web");
    assert(request.header("Content-Type") == "application/json");
    assert(request.body == body);

    const auto response =
        reactor_http_kit::http::HttpResponse::json(200, R"({"ok":true})").toString();
    assert(response.find("HTTP/1.1 200 OK") != std::string::npos);
    assert(response.find("application/json") != std::string::npos);
    assert(response.ends_with(R"({"ok":true})"));
    return 0;
}
