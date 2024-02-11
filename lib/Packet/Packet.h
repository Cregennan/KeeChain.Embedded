#ifndef PACKETLIB
#define PACKETLIB
#include <Arduino.h>
#include <memory>
#include <functional>

static const uint16_t MAGICHEADER = 0xBABA;

static const size_t CONTENT_LENGTH_LIMIT_BYTES = 256;

enum PacketType : uint8_t{
    DISCOVER,
    ALIVE,
    ACK,
    DATA
};

enum RECIEVE_STATUS : uint8_t{
    UNSPECIFIED,
    OK,
    TIMEOUT_REACHED,
    CRC_MISMATCH,
    CONTENT_LENGTH_EXCEEDED
};

enum SEND_STATUS : uint8_t{
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

struct InnerPacket {
    const uint16_t MAGIC = MAGICHEADER;
    int8_t METADATA;
    PacketType TYPE;
    size_t CONTENT_LENGTH;
    uint32_t CRC;
    std::unique_ptr<uint8_t> CONTENT;
};

class PacketManager{
    public: 
        SEND_STATUS send(std::vector<uint8_t> data);

        std::pair<RECIEVE_STATUS, std::shared_ptr<Packet>> recieve();

        PacketManager(
            std::function<uint8_t(void)> read, 
            std::function<void(uint8_t)> write,
            std::function<size_t(void)> available, 
            time_t pollingTimeout = 0, 
            time_t pollingDelayMs = 20);

    private:
        std::function<uint8_t(void)> read;
        std::function<void(uint8_t)> write;
        std::function<size_t(void)> available; 
        time_t pollingTimeout;
        time_t pollingDelayMs;
};


#endif