#include <Packet.h>
#include <utility>
#include <vector>
#include <packet_extensions.cpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-extensions"

SEND_STATUS PacketManager::send(std::vector<uint8_t>& vector){
    auto packet = std::shared_ptr<packp::InnerPacketV1>(new packp::InnerPacketV1());
    auto contentPointer = vector.data();
    packet->CONTENT = std::shared_ptr<uint8_t>(contentPointer);
    packet->CONTENT_LENGTH = vector.size();
    packet->TYPE = PacketType::DATA;

    return packet_manager::send_packet(packet, this->params);
}

std::pair<RECEIVE_STATUS, Packet> PacketManager::receive(){

    while(true){
        auto [status, innerPacket] = packet_manager::receive_packet(this->params);
        Packet packet;

        if (status == RECEIVE_STATUS::CRC_MISMATCH){
            if (this->last_sent_packet == nullptr){
                return std::make_pair(RECEIVE_STATUS::INTERNAL_STATE_CORRUPTED, packet);
            }else{
                auto p = this->last_sent_packet;
                auto vec = std::vector<uint8_t>(p->CONTENT.get(), p->CONTENT.get() + p->CONTENT_LENGTH);
                this->send(vec);
                continue;
            }
        }

        if (status != RECEIVE_STATUS::OK){
            return std::make_pair(status, packet);
        }

        if (innerPacket->TYPE == PacketType::DISCOVER || innerPacket->TYPE == PacketType::ALIVE){
            auto innerPacketACK = new packp::InnerPacketV1();
            innerPacketACK->TYPE = PacketType::ACK;
            packet_manager::send_packet(std::shared_ptr<packp::InnerPacketV1>(innerPacketACK), this->params);
            continue;
        }

        packet.TYPE = innerPacket->TYPE;
        packet.CONTENT = std::vector<uint8_t>(innerPacket->CONTENT.get(), innerPacket->CONTENT.get() + innerPacket->CONTENT_LENGTH);
        packet.METADATA = innerPacket->METADATA;
        return std::make_pair(status, packet);
    }
}

__attribute__((unused)) PacketManager::PacketManager(const PacketManagerParams params) {
    this->params = params;
}

#pragma clang diagnostic pop