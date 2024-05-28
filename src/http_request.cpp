#include "http_request.h"
#include "lwip/altcp.h"
#include "lwip/pbuf.h"

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

void request::abort_request()
{
  if (buffer)
    buffer = pbuf_free_header(buffer, buffer->tot_len);
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

} // namespace http
