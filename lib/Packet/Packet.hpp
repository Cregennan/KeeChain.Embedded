#ifndef PACKETLIB
#define PACKETLIB
#include <Arduino.h>
#include <memory>
#include <functional>

static const uint16_t MAGICHEADER = 0xBABA;

enum PacketType : uint8_t{
    DISCOVER,
    ALIVE,
    ACK,
    DATA
};

struct Packet {
    uint16_t MAGIC_HEAD;
    int8_t VERSION;
    PacketType TYPE;
    size_t CONTENT_LENGTH;
    std::unique_ptr<uint8_t> CONTENT;
    uint32_t CRC;
};

void write_packet(std::shared_ptr<Packet> packet, std::function<void(uint8_t)> write);

void write_data(std::unique_ptr<uint8_t> data, size_t length, std::function<void(uint8_t)> write);

#endif