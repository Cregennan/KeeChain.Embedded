#ifndef PACKETLIB
#define PACKETLIB
#include <Arduino.h>
#include <memory>
#include <functional>
#include <vector>
#include <deque>

enum PacketType : uint8_t{
    DISCOVER,
    ALIVE,
    ACK,
    DATA,
    FIN
};

static const uint16_t MAGIC_HEADER = 0xBABA;

static const std::deque<uint8_t> MAGIC_HEADER_BYTEWISE = {0xBA, 0xBE};

static const size_t CONTENT_LENGTH_LIMIT_BYTES = 256;

enum class RECEIVE_STATUS : uint8_t{
    UNSPECIFIED,
    OK,
    TIMEOUT_REACHED,
    CRC_MISMATCH,
    CONTENT_LENGTH_EXCEEDED,
    MALFORMED
};

enum class SEND_STATUS : uint8_t{
    UNSPECIFIED,
    OK,
    CONTENT_LENGTH_EXCEEDED,
    INVALID_CONTENT
};

struct Packet{
    int8_t METADATA;
    PacketType TYPE;
    std::vector<uint8_t> CONTENT;
};

struct PacketManagerParams{
    uint8_t (*read)();
    void (*write)(uint8_t);
    size_t (*available)();
    time_t pollingTimeout = 0;
    time_t pollingDelayMs = 20;
};

class PacketManager{
    public: 
        SEND_STATUS send(const std::vector<uint8_t>& data);

        std::pair<RECEIVE_STATUS, std::shared_ptr<Packet>> receive();

        explicit PacketManager(const PacketManagerParams& params);

    private:
        uint8_t (*read)();
        void (*write)(uint8_t);
        size_t (*available)();
        time_t pollingTimeout;
        time_t pollingDelayMs;
};

#endif