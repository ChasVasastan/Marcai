#include "http_request.h"

namespace http {

http::request::request() {
  status = 0;
  content_length = -1;
  buffer = nullptr;
  body_rx = 0;
  chunk_size = 0;
  state = http::state::STATUS;
}

void http::request::add_header(std::string header) {
  if (header.empty()) {
    printf("HTTP headers received\n");
    state = http::state::BODY;
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

size_t http::request::transfer_body(int offset, size_t size) {
  std::vector<uint8_t> partial_body(size);
  pbuf_copy_partial(buffer, partial_body.data(), partial_body.size(), offset);
  size_t consumed = callback_body(this, partial_body);
  body_rx += consumed;
  if (body_rx == content_length) {
    state = http::state::DONE;
  }
  return offset + consumed;
}

size_t http::request::transfer_chunked(int offset) {
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
        state = http::state::DONE;
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

} // namespace http
