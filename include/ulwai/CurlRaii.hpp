/**
 * @file CurlRaii.hpp
 * @brief Zero-overhead RAII smart pointer wrappers for libcurl easy handles and header lists.
 *
 * Guarantees exception-safe and early-return cleanup of CURL* (`curl_easy_cleanup`)
 * and curl_slist* (`curl_slist_free_all`) without resource leakage.
 */

#pragma once

#include <curl/curl.h>
#include <memory>

namespace ulwai::net {

/**
 * @brief Custom deleter functor for CURL easy handles.
 */
struct CurlDeleter {
    void operator()(CURL* curl) const noexcept {
        if (curl) curl_easy_cleanup(curl);
    }
};

/**
 * @brief Custom deleter functor for curl_slist linked lists.
 */
struct CurlSlistDeleter {
    void operator()(curl_slist* list) const noexcept {
        if (list) curl_slist_free_all(list);
    }
};

/**
 * @brief RAII unique ownership wrapper for CURL easy handle.
 */
using UniqueCurl = std::unique_ptr<CURL, CurlDeleter>;

/**
 * @brief RAII unique ownership wrapper for curl_slist header list.
 */
using UniqueCurlSlist = std::unique_ptr<curl_slist, CurlSlistDeleter>;

/**
 * @brief Safely append an HTTP header to a UniqueCurlSlist without leaking memory on realloc.
 * @param list Reference to the unique pointer holding the list.
 * @param str Null-terminated header string (e.g., "Content-Type: application/json").
 */
inline void appendHeader(UniqueCurlSlist& list, const char* str) {
    curl_slist* raw = list.release();
    raw = curl_slist_append(raw, str);
    list.reset(raw);
}

} // namespace ulwai::net
