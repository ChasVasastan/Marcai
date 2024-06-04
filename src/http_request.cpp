#include <sstream>

#include "hardware/sync.h"
#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "pico/time.h"

#include "http_request.h"

namespace http {

request::request() {
  status = 0;
  content_length = -1;
  buffer = nullptr;
  body_rx = 0;
  chunk_size = 0;
  state = state::STATUS;
}

request::~request() {
  if (buffer)
    pbuf_free_header(buffer, buffer->tot_len);
}

void request::add_header(std::string header) {
  if (header.empty()) {
    state = state::BODY;
    return;
  }

  if (auto npos = header.find(": "); npos != std::string::npos) {
    std::string name = header.substr(0, npos);
    std::string value = header.substr(npos + 2);
    for (char &c : name) {
      c = std::tolower(c);
    }
    response_headers[name] = value;
    if (callback_header)
      callback_header(this, name, value);

    if (name == "content-length") {
      content_length = std::atoi(value.c_str());
    } else if (name == "transfer-encoding" && value == "chunked") {
      content_length = -1;
    }
  } else {
    panic("Invalid header\n%s", header.c_str());
  }
}

size_t request::transfer_body(int offset, size_t size) {
  std::vector<uint8_t> partial_body(size);
  pbuf_copy_partial(buffer, partial_body.data(), partial_body.size(), offset);
  if (body_rx + size == content_length)
    state = state::DONE;

  int consumed = 0;
  if (callback_body) {
    consumed = callback_body(this, partial_body);
    body_rx += consumed;
  }

  if (content_length > 0) {
    int part = (body_rx + buffer->tot_len) - consumed;
    printf("Got %d bytes of %d (%.2f%%)\n", part, content_length,
           ((float)part / (float)content_length) * 100);
  }

  return offset + consumed;
}

void request::abort()
{
  printf("Abort request\n");
  state = state::FAILED;
  altcp_close(pcb);
}

size_t request::transfer_chunked(int offset) {
  if (chunk_size == 0) {
    // Get next chunk size
    uint16_t end = pbuf_memfind(buffer, "\r\n", 2, offset);
    if (end < 0xffff) {
      std::string chunk_size_string(end - offset, '\0');
      pbuf_copy_partial(buffer, chunk_size_string.data(),
                        chunk_size_string.size(), offset);
      chunk_size = std::stoi(chunk_size_string.c_str(), nullptr, 16);
      if (chunk_size == 0) {
        // Last chunk, ignore trailing headers and close connection
        printf("Last chunk, closing connection...\n");
        content_length = body_rx;
        state = state::DONE;
        return offset;
      }
      offset += chunk_size_string.size() + 2;
    } else {
      // Chunk size not found. Wait for more data...
      printf("Wait for more data...\n");
      return offset;
    }
  }

  while (chunk_size > 0) {
    size_t size = std::min(buffer->tot_len - offset, chunk_size);
    if (size == 0) {
      // End of buffer. Wait for more data...
      printf("Wait for more data...\n");
      return offset;
    }
    offset = transfer_body(offset, size);
    chunk_size -= size;
  }

  // End of chunk, remove crlf
  return transfer_chunked(offset + 2);
}

size_t request::read(uint8_t *dst, size_t size) {
  size = peek(dst, size);
  return skip(size);
}

size_t request::peek(uint8_t *dst, size_t size) {
  if (state == state::STATUS || state == state::HEADERS)
    return 0;

  size = MIN(size, buffer->tot_len);
  return pbuf_copy_partial(buffer, dst, size, 0);
}

size_t request::skip(size_t size) {
  if (state == state::STATUS || state == state::HEADERS)
    return 0;

  size = MIN(size, buffer->tot_len);
  buffer = pbuf_free_header(buffer, size);
  altcp_recved(pcb, size);
  body_rx += size;
  return size;
}


void request::send(http::request *req) {
  // Resolve DNS
  ip_addr_t addr;
  err_t err = dns_gethostbyname(req->hostname.c_str(), &addr, dns_resolve_callback, req);
  if (err == ERR_INPROGRESS) {
    printf("DNS resolution in progress...\n");
  } else if (err == ERR_OK) {
    printf("Target IP: %s\n", ipaddr_ntoa(&addr));
    tls_tcp_setup(req, &addr);
  } else {
    printf("DNS resolve failed/error\n");
  }
}


  // Function to handle data being transmitted
err_t request::recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err) {
  auto *req = reinterpret_cast<http::request *>(arg);
  uint16_t offset = 0;

  if (p == NULL) {
    printf("No data received or connection closed\n");
    return ERR_OK;
  }

  if (req->buffer == NULL)
    req->buffer = p;
  else
    pbuf_cat(req->buffer, p);

  if (req->state == http::state::STATUS) {
    uint16_t end = pbuf_memfind(req->buffer, "\r\n", 2, offset);
    if (end < 0xffff){
      std::string status_line(end, '\0');
      pbuf_copy_partial(req->buffer, status_line.data(), end, offset);
      printf("< %s\n", status_line.c_str());
      if (auto npos = status_line.find(" "); npos != std::string::npos) {
        std::string version = status_line.substr(0, npos);
        int status = std::atoi(status_line.substr(npos).c_str());
        req->status = status;
        if (version != req->version){
          panic("Response version %s != HTTP/1.1", version.c_str());
        }
        req->state = http::state::HEADERS;
      } else {
        panic("Invalid status line\n%s", status_line.c_str());
      }
      offset = end + 2;
    } else {
      // TODO: handle status not in payload
      panic("Status outside this buffer");
    }
  }

  if (req->state == http::state::HEADERS) {
    uint16_t end;
    do {
      end = pbuf_memfind(req->buffer, "\r\n", 2, offset);
      if (end < 0xffff) {
        // Found HTTP header
        std::string header(end - offset, '\0');
        pbuf_copy_partial(req->buffer, header.data(), header.size(), offset);
        printf("< %s\r\n", header.c_str());
        offset = end + 2;

        req->add_header(header);
      }
    } while (end < 0xffff && req->state == http::state::HEADERS);
  }

  if (req->state == http::state::BODY) {
    // Call the callback body function
    if (req->content_length >= 0)
      offset = req->transfer_body(offset, req->buffer->tot_len - offset);
    else
      offset = req->transfer_chunked(offset);
  }

  // Tells the underlying TCP mechanism that the data has been received
  altcp_recved(pcb, offset);
  req->buffer = pbuf_free_header(req->buffer, offset);

  if (req->state == http::state::DONE) {
    printf("Body transferred, closing connection (%d bytes)\n",req->body_rx);
    if (req->callback_result)
      req->callback_result(req);
    return altcp_close(pcb);
  }

  if (req->state == http::state::FAILED) {
    printf("Connection failed, closing...\n");
    pbuf_free(req->buffer);
    return altcp_close(pcb);
  }

  return ERR_OK;
}

// Function to handle connection events
err_t request::altcp_client_connected(void *arg, struct altcp_pcb *pcb, err_t err)
{
  auto *req = reinterpret_cast<http::request*>(arg);

  if (err == ERR_OK) {
    printf("Connection established!\n");
    std::stringstream ss;

    // request line
    ss << req->method << " " << req->path << " " << req->version << "\r\n";
    printf("> %s", ss.str().c_str());

    if (req->headers.count("host") == 0)
      req->headers["host"] = req->hostname;

    if (req->method == "POST" || req->method == "PUT" || req->method == "PATCH")
    {
      req->headers["content-length"] = std::to_string(req->body.size());
    }

    // request headers
    for (auto [name,value] : req->headers) {
      printf("> %s: %s\r\n", name.c_str(), value.c_str());
      ss << name << ": " << value << "\r\n";
    }

    printf("After headers sent\n");

    // End of request headers
    ss << "\r\n";

    printf(">\r\n");
    std::string request_string = ss.str();
    err = altcp_write(pcb, request_string.c_str(), request_string.length(), 0);
    err = altcp_output(pcb);

    // request body
    // err = altcp_write(pcb, req->body.data(), req->body.size(), 0);
    // err = altcp_output(pcb);

  } else {
    printf("Error on connect: %d\n", err);
  }

  return ERR_OK;
}

// Function to set up TLS
void request::tls_tcp_setup(http::request *req, const ip_addr_t *addr)
{
    // Set up TLS
    static struct altcp_tls_config *tls_config = altcp_tls_create_config_client(NULL, 0);
    struct altcp_pcb *pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY);
    altcp_recv(pcb, recv);
    altcp_arg(pcb, req);
    req->pcb = pcb;

    // Connect to host with TLS/TCP
    printf("Attempting to connect to %s with IP address %s\n",
           req->hostname.c_str(), ipaddr_ntoa(addr));
    auto ctx = reinterpret_cast<mbedtls_ssl_context*>(pcb);
    mbedtls_ssl_set_hostname(ctx, req->hostname.c_str());
    err_t err = altcp_connect(pcb, addr, 443, altcp_client_connected);
    if (err != ERR_OK)
    {
        printf("TLS connection failed, error: %d\n", err);
    }
  }

// Callback for DNS lookup
void request::dns_resolve_callback(const char *name, const ip_addr_t *addr, void *arg)
{
  auto *req = reinterpret_cast<http::request*>(arg);
  if (addr != NULL) {
    printf("DNS lookup successful: %s, IP address is %s\n",
           name, ipaddr_ntoa(addr));
    tls_tcp_setup(req, addr);
  }
}

} // namespace http
