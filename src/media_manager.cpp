#include "media_manager.h"

#include "lwipopts.h"
#include "lwip/altcp.h"
#include "lwip/dns.h"
#include "lwip/altcp_tls.h"

#include "audio.h"
#include "wifi.h"
#include "http_client.h"


static size_t decode_mp3(http::request *req, std::vector<uint8_t> data)
{
  // Find next frame in buffer
  size_t offset = MP3FindSyncWord(data.data(), data.size());

  while (offset < data.size())
  {
    int read;
    do
    {
      // Decode data while audio buffers are available
      read = g_audio.stream_decode(data.data() + offset,
                                   data.size() - offset);
    } while (read < 0);

    if (read == 0)
    {
      // Could not decode because of insufficient data
      return offset;
    }

    offset += read;
  }

  return offset;
}

void media_manager::play(http::url url)
{
  // Change this to the desired target
  char host[] = "google-translate1.p.rapidapi.com";
  // https://marcai.blob.core.windows.net/audio/mono/YourMom.mp3
  http::client http_client;
  http::request req[4];
  req[0].client = &http_client;
  req[0].hostname = host;
  req[0].path = "/language/translate/v2/detect";
  req[0].method = "GET";
  req[0].callback_body = decode_mp3;

  http_client.request(&req[0]);
}

http::url media_manager::generate_url(std::string keywords)
{
  return http::url();
}

void media_manager::pause()
{
  
}

void media_manager::next()
{

}

void media_manager::previous()
{

}

void media_manager::init()
{
  audio.init_i2s();
}
