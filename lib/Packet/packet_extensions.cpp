#include <Packet.h>
#include <vector>
#include <deque>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-extensions"



namespace packet{

    struct InnerPacketV1 {
        const uint16_t MAGIC = MAGIC_HEADER;
        int8_t PACKET_VERSION = 1;
        int8_t METADATA{};
        PacketType TYPE;
        size_t CONTENT_LENGTH{};
        uint32_t CRC{};
        std::unique_ptr<uint8_t> CONTENT;
    };


    inline std::pair<RECEIVE_STATUS, std::shared_ptr<InnerPacketV1>> error_status(RECEIVE_STATUS status){
        return std::make_pair(status, std::nullptr_t());
    }

    inline decltype(InnerPacketV1::CRC) calculate_crc(const uint8_t* pointer, size_t n){
        decltype(InnerPacketV1::CRC) crc = 0;
        for (size_t i = 0; i < n; i++)
        {
            crc ^= pointer[i] << (i % sizeof(InnerPacketV1::CRC));
        }

        return crc;
    }

    inline RECEIVE_STATUS read_magic_bytes(
            PacketManagerParams & params,
            uint32_t & polling_started_time)
    {
        std::deque<uint8_t> buffer;
        while(true){
            if (millis() - polling_started_time > params.pollingTimeout){
                return RECEIVE_STATUS::TIMEOUT_REACHED;
            }

            if (params.available() <= 0){
                delay(params.pollingDelayMs);
                continue;
            }

            if(buffer.size() < sizeof(MAGIC_HEADER)){
                buffer.push_back(params.read());
            }
            if (buffer == MAGIC_HEADER_BYTEWISE){
                buffer.clear();
                break;
            }else{ // Если байты в буфере не совпали - удаляем первый, готовимся считать очередной
                buffer.erase(buffer.begin());
            }

            delay(params.pollingDelayMs);
        }
        return RECEIVE_STATUS::OK;
    }

    inline RECEIVE_STATUS read_n_bytes(
            uint8_t * vector,
            size_t n,
            PacketManagerParams & params,
            uint32_t & polling_started)
    {
        for(auto i = 0; i < n; i++){
            if (millis() - polling_started > params.pollingTimeout){
                return RECEIVE_STATUS::TIMEOUT_REACHED;
            }
            if (params.available() > 0){
                vector[i] = params.read();
            }
            delay(params.pollingDelayMs);
        }
        return RECEIVE_STATUS::OK;
    };
}

namespace packet_manager{

    std::pair<RECEIVE_STATUS, std::shared_ptr<packet::InnerPacketV1>> receive_packet(PacketManagerParams & params){

        // Polling timeouts
        auto polling_start_time = millis();

        //Собираемый пакет
        auto packet = new packet::InnerPacketV1();
        auto magicBytesStatus = packet::read_magic_bytes(params, polling_start_time);
        if (magicBytesStatus != RECEIVE_STATUS::OK){
            return packet::error_status(magicBytesStatus);
        }

        //Чтение заголовка
        auto header_pointer = reinterpret_cast<uint8_t*>(packet) + sizeof(packet::InnerPacketV1::MAGIC);
        auto hl_without_magic = sizeof(packet::InnerPacketV1) - sizeof(packet::InnerPacketV1::CONTENT) - sizeof(packet::InnerPacketV1::MAGIC);

        auto header_reading_status = packet::read_n_bytes(header_pointer, hl_without_magic, params, polling_start_time);

        if (header_reading_status != RECEIVE_STATUS::OK){
            return packet::error_status(header_reading_status);
        }

        if (packet->CONTENT_LENGTH < 1){
            return packet::error_status(RECEIVE_STATUS::MALFORMED);
        }
        if (packet->CONTENT_LENGTH > CONTENT_LENGTH_LIMIT_BYTES){
            return packet::error_status(RECEIVE_STATUS::CONTENT_LENGTH_EXCEEDED);
        }

        //Чтение содержимого
        auto content_pointer = new uint8_t[packet->CONTENT_LENGTH];
        auto content_reading_status = packet::read_n_bytes(content_pointer, packet->CONTENT_LENGTH, params, polling_start_time);
        if (content_reading_status != RECEIVE_STATUS::OK){
            return packet::error_status(content_reading_status);
        }

        packet->CONTENT = std::unique_ptr<uint8_t>(content_pointer);

        if (packet->CRC != packet::calculate_crc(packet->CONTENT.get(), packet->CONTENT_LENGTH)){
            return packet::error_status(RECEIVE_STATUS::CRC_MISMATCH);
        }

        return std::make_pair(RECEIVE_STATUS::OK, std::shared_ptr<packet::InnerPacketV1>(packet));
    }
}
#pragma clang diagnostic pop