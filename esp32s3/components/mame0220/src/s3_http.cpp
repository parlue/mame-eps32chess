// The ESP frontend supplies its own web service. MAME's optional internal
// desktop HTTP server stays disabled, without allocating an ASIO context.
// The original allocates that context even with active == false.
#include "xtensa_posix_shim.h"
#include "emu.h"
#include "http.h"
#pragma GCC diagnostic ignored "-Wuninitialized"
// Complete member types for unique_ptr destruction. Neither is constructed.
#include "server_ws_impl.hpp"
#include "server_http_impl.hpp"
#include <stdexcept>

http_manager::http_manager(bool active, short, const char *root)
    : m_active(false), m_root(root ? root : "")
{
    if (active)
        throw std::runtime_error("MAME internal HTTP is unavailable; use the ESP frontend server");
}
http_manager::~http_manager() = default;
void http_manager::clear() { }
void http_manager::add_http_handler(const std::string &, http_handler) { }
void http_manager::remove_http_handler(const std::string &) { }
