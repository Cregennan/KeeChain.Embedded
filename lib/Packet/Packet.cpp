#include <Packet.h>
#include <utility>
#include <vector>
#include <packet_extensions.cpp>

#pragma region LowLevel

//TODO: переписать SEND
void inner_send_packet(std::shared_ptr<packet::InnerPacketV1> packet, std::function<void(uint8_t)> write){
    auto head = reinterpret_cast<uint8_t*>(packet.get());
    auto headerLength = sizeof(packet::InnerPacketV1) - sizeof(packet->CRC) - sizeof(packet->CONTENT);

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

#pragma endregion

#pragma region PacketManager

SEND_STATUS PacketManager::send(const std::vector<uint8_t>& data){
    
}

std::pair<RECEIVE_STATUS, std::shared_ptr<Packet>> PacketManager::receive(){

}

PacketManager::PacketManager(const PacketManagerParams& params) {
    this->available = params.available;
    this->read = params.read;
    this->write = params.write;
    this->pollingDelayMs = params.pollingDelayMs;
    this->pollingTimeout = params.pollingTimeout;
}

#pragma endregion