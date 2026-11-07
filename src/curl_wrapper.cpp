#include "curl_wrapper.hpp"
#include <curl/curl.h>
#include <cstdint>

static size_t __WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    auto* buffer = static_cast<std::string*>(userp);    
    auto* data_ptr = static_cast<uint8_t*>(contents);
    buffer->insert(buffer->end(), data_ptr, data_ptr + total_size);    
    return total_size;
}

std::string download(const std::string& uri) {
    CURL* curl = curl_easy_init();
    if (!curl) { return {}; }

    std::string data;

    curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
    
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "LSWE-Example/1.0");

    CURLcode res = curl_easy_perform(curl);
    
    curl_easy_cleanup(curl);

    return res == CURLE_OK ? data : std::string{};
}
