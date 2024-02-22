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
    DATA
};


static const uint16_t MAGIC_HEADER = 0xBABA;

static const size_t CONTENT_LENGTH_LIMIT_BYTES = 256;

enum class RECEIVE_STATUS : uint8_t{
    UNSPECIFIED,
    OK,
    TIMEOUT_REACHED,
    CRC_MISMATCH,
    CONTENT_LENGTH_EXCEEDED,
    MALFORMED,
    INTERNAL_STATE_CORRUPTED
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
    uint8_t (*read)(){};
    void (*write)(uint8_t){};
    size_t (*available)(){};
    time_t pollingTimeout = 0;
    time_t pollingDelayMs = 20;
};

namespace packp{
    struct InnerPacketV1 {
        const uint16_t MAGIC = MAGIC_HEADER;
        int8_t PACKET_VERSION = 1;
        int8_t METADATA{};
        PacketType TYPE;
        size_t CONTENT_LENGTH{};
        uint32_t CRC{};
        std::shared_ptr<uint8_t> CONTENT;
    };
}

class PacketManager{

    enum class PacketManagerState : uint8_t{
        LISTEN,
        ESTABLISHED,
        CLOSED
    };

    public: 
        SEND_STATUS send(std::vector<uint8_t>& vector);

        std::pair<RECEIVE_STATUS, Packet> receive();

    __attribute__((unused)) explicit PacketManager(PacketManagerParams params);

    private:
        std::shared_ptr<packp::InnerPacketV1> last_sent_packet;
        PacketManagerParams params;
};

#endif