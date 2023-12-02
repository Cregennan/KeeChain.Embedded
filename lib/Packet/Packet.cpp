#include <Packet.hpp>

void write_packet(std::shared_ptr<Packet> packet, std::function<void(uint8_t)> write){

    packet->MAGIC_HEAD = MAGICHEADER;
    auto head = reinterpret_cast<uint8_t*>(packet.get());
    auto headerLength = sizeof(Packet) - sizeof(packet->CRC) - sizeof(packet->CONTENT);

    for (size_t i = 0; i < headerLength; i++)
    {
        write(head[i]);
    }
    
    uint32_t crc = 0;

    for (size_t i = 0; i < packet->CONTENT_LENGTH; i++)
    {
        auto value = packet->CONTENT.get()[i];
        crc ^= value << (i % (sizeof(crc) / sizeof(uint8_t)));
        write(value);
    }

    auto crcPointer = reinterpret_cast<uint8_t*>(&crc);
    for (size_t i = 0; i < sizeof(crc); i++)
    {
        write(crcPointer[i]);
    }
}

void write_data(std::unique_ptr<uint8_t> data, size_t length, std::function<void(uint8_t)> write){
    Packet packet;
    packet.CONTENT = std::move(data);
    packet.CONTENT_LENGTH = length;
    packet.TYPE = DATA;
    write_packet(std::shared_ptr<Packet>(&packet), write);
}