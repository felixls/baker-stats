#include "tzstats.h"
#include <HTTPClient.h>
#include <ArduinoStreamParser.h>

class HeadHandler : public JsonHandler {
  public:
    long block = 0;
    long cycle = 0;
    String timestamp;
    uint16_t state = 0;
    void value(ElementPath path, ElementValue value) {
        const char *currentKey = path.getKey();
        if (currentKey[0] != '\0') {
            if (!done()) {
                if (strcmp(currentKey, "height") == 0) {
                    block = value.getInt();
                    state++;
                } else if (strcmp(currentKey, "timestamp") == 0) {
                    timestamp = value.getString();
                    state++;
                } else if (strcmp(currentKey, "cycle") == 0) {
                    cycle = value.getInt();
                    state++;
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
    bool done() { return state == 3; }
};

bool Tzstats::getHead() {
    Sprint(F("[HTTP] begin...\n"));

    HTTPClient http;

    http.begin(F("https://api.tzstats.com/explorer/tip"));

    Sprint(F("[HTTP] GET...\n"));
    int httpCode = http.GET();
    if (httpCode > 0) {
        Sprintf("[HTTP] GET head... code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK) {
            int len = http.getSize();

            uint8_t buff[128] = {0};

            WiFiClient *stream = http.getStreamPtr();

            HeadHandler handler;
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

            this->block = handler.block;
            this->cycle = handler.cycle;
            String timestamp = handler.timestamp;
            int pos = timestamp.indexOf("T");
            if (pos > 0) {
                this->date = timestamp.substring(0, pos);
                this->time = timestamp.substring(pos + 1);
            }

            Sprintln(this->cycle);
            Sprintln(this->block);
            Sprintln(this->date);
            Sprintln(this->time);

            Sprintln();
            Sprint(F("[HTTP] connection closed or file end.\n"));
        }
    } else {
        Sprintf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();

    return true;
}

class CycleHandler : public JsonHandler {
  public:
    long startBlock = 0;
    long endBlock = 0;
    uint16_t state = 0;
    void value(ElementPath path, ElementValue value) {
        const char *currentKey = path.getKey();
        if (currentKey[0] != '\0') {
            if (!done()) {
                if (strcmp(currentKey, "start_height") == 0) {
                    startBlock = value.getInt();
                    state++;
                } else if (strcmp(currentKey, "end_height") == 0) {
                    endBlock = value.getInt();
                    state++;
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
    bool done() { return state == 2; }
};

bool Tzstats::getCycleHead() {
    Sprint(F("[HTTP] begin...\n"));

    HTTPClient http;

    String url = F("https://api.tzstats.com/explorer/cycle/");
    url += String(this->cycle);

    http.begin(url);

    Sprint("[HTTP] GET...\n");
    int httpCode = http.GET();
    if (httpCode > 0) {
        Sprintf("[HTTP] GET cycle... code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK) {
            int len = http.getSize();

            uint8_t buff[128] = {0};

            WiFiClient *stream = http.getStreamPtr();

            CycleHandler handler;
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

            this->start = handler.startBlock;
            this->end = handler.endBlock;

            Sprintln(this->start);
            Sprintln(this->end);

            Sprintln();
            Sprint(F("[HTTP] connection closed or file end.\n"));
        }
    } else {
        Sprintf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();

    return true;
}

class RightsHandler : public JsonHandler {
  private:
    int element = 0;

  public:
    int index = 0;
    Right rights[40];
    void value(ElementPath path, ElementValue value) {
        switch (element) {
            case 1:
                rights[index].baking = (strcmp(value.getString(), "baking") == 0);
                break;
            case 2:
                rights[index].block = value.getInt();
                break;
            case 4:
                rights[index].priority = value.getInt();
                break;
            case 6:
                rights[index].used = value.getInt() == 1 ? true : false;
                break;
            case 7:
                rights[index].lost = value.getInt() == 1 ? true : false;
                break;
            case 8:
                rights[index].stolen = value.getInt() == 1 ? true : false;
                break;
            case 9:
                rights[index].missed = value.getInt() == 1 ? true : false;
                break;
            case 12:
                rights[index].bondMiss = value.getInt() == 1 ? true : false;
                break;
            default:
                break;
        }
        element++;
    }

    void endDocument() {}
    void startDocument() {}
    void startObject(ElementPath path) {}
    void endObject(ElementPath path) {}
    void startArray(ElementPath path) { element = 0; }
    void endArray(ElementPath path) {
        if (element > 0) {

            index++;
        }
    }
    void whitespace(char c) {}
};

bool Tzstats::getRights() {
    Sprint("[HTTP] begin...\n");

    HTTPClient http;

    String url = F("https://api.tzstats.com/tables/rights?address=");
    url += F(BAKER_ADDRESS);
    url += "&cycle=";
    url += String(this->cycle);

    http.begin(url);

    Sprint("[HTTP] GET...\n");
    int httpCode = http.GET();
    if (httpCode > 0) {
        Sprintf("[HTTP] GET rights... code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK) {
            int len = http.getSize();

            uint8_t buff[128] = {0};

            WiFiClient *stream = http.getStreamPtr();

            RightsHandler handler;
            JsonStreamingParser parser;
            parser.setHandler(&handler);

            while (http.connected() && stream->available() && (len > 0 || len == -1)) {
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

            if (handler.index > 0) {
                for (int i = 0; i < handler.index - 1; i++) {
                    Sprintln(handler.rights[i].block);
                    Sprintln(handler.rights[i].baking);
                    Sprintln(handler.rights[i].used);
                    Sprintln(handler.rights[i].missed);
                }
                numRights = handler.index;
                rights = handler.rights;
            }

            Sprintln();
            Sprint(F("[HTTP] connection closed or file end.\n"));
        }
    } else {
        Sprintf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();

    return true;
}
