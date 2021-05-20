#include "apitez.h"
#include <HTTPClient.h>
#include <ArduinoStreamParser.h>

class ApiHandler : public JsonHandler {
  private:
    long block = 0;
    bool finished = false;

  public:
    void value(ElementPath path, ElementValue value) {
        const char *currentKey = path.getKey();
        if (currentKey[0] != '\0') {
            if (!finished) {
                if (strcmp(currentKey, "level") == 0) {
                    block = value.getInt();
                    finished = true;
                }
            }
        }
    }

    void endDocument() {}
    void startDocument() {}
    void startObject(ElementPath path) {}
    void endObject(ElementPath path) {}
    void startArray(ElementPath path) {}
    void endArray(ElementPath path) {}
    void whitespace(char c) {}

    long getBlock() { return block; }
    bool done() { return finished; }
};

long Apitez::get() {
    long status_blocks = 0;
    Sprint(F("[HTTP] begin...\n"));

    HTTPClient http;

    http.begin(F("https://api.tez.ie/rpc/mainnet/chains/main/blocks/head/helpers/current_level"));

    Sprint("[HTTP] GET...\n");
    int httpCode = http.GET();
    if (httpCode > 0) {
        Sprintf("[HTTP] GET... code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK) {
            int len = http.getSize();

            uint8_t buff[128] = {0};

            WiFiClient *stream = http.getStreamPtr();

            ApiHandler handler;
            JsonStreamingParser parser;
            parser.setHandler(&handler);

            while (!handler.done() && http.connected() && stream->available() && (len > 0 || len == -1)) {
                size_t size = stream->available();

                if (size) {
                    int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                    if (c) {
                        int i = 0;
                        while (c-- > 0) {
                            parser.parse(buff[i++]);
                        }

                        if (len > 0) {
                            len -= c;
                        }
                    }
                }
                delay(1);
            }

            status_blocks = handler.getBlock();

            Sprintln(status_blocks);

            Sprintln();
            Sprint(F("[HTTP] connection closed or file end.\n"));
        }
    } else {
        Sprintf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();

    return status_blocks;
}
