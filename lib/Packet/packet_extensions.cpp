#include <Packet.h>
#include <vector>
#include <deque>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-extensions"

static const std::deque<uint8_t> MAGIC_HEADER_DEQUE = {0xBA, 0xBE};

namespace packp{

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
            if (buffer == MAGIC_HEADER_DEQUE){
                buffer.clear();
                break;
            }else{ // Если байты в буфере не совпали - удаляем первый, готовимся считать очередной
                buffer.pop_front();
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

    //Получение пакета
    std::pair<RECEIVE_STATUS, std::shared_ptr<packp::InnerPacketV1>> receive_packet(PacketManagerParams & params){

        // Polling timeouts
        auto polling_start_time = millis();

        //Собираемый пакет
        auto packet = new packp::InnerPacketV1();
        auto magicBytesStatus = packp::read_magic_bytes(params, polling_start_time);
        if (magicBytesStatus != RECEIVE_STATUS::OK){
            return packp::error_status(magicBytesStatus);
        }

        //Чтение заголовка
        auto header_pointer = reinterpret_cast<uint8_t*>(packet) + sizeof(packp::InnerPacketV1::MAGIC);
        auto hl_without_magic = sizeof(packp::InnerPacketV1) - sizeof(packp::InnerPacketV1::CONTENT) - sizeof(packp::InnerPacketV1::MAGIC);

        auto header_reading_status = packp::read_n_bytes(header_pointer, hl_without_magic, params, polling_start_time);

        if (header_reading_status != RECEIVE_STATUS::OK){
            return packp::error_status(header_reading_status);
        }

        if (packet->CONTENT_LENGTH < 1){
            return packp::error_status(RECEIVE_STATUS::MALFORMED);
        }
        if (packet->CONTENT_LENGTH > CONTENT_LENGTH_LIMIT_BYTES){
            return packp::error_status(RECEIVE_STATUS::CONTENT_LENGTH_EXCEEDED);
        }

        //Чтение содержимого
        auto content_pointer = new uint8_t[packet->CONTENT_LENGTH];
        auto content_reading_status = packp::read_n_bytes(content_pointer, packet->CONTENT_LENGTH, params, polling_start_time);
        if (content_reading_status != RECEIVE_STATUS::OK){
            return packp::error_status(content_reading_status);
        }

        packet->CONTENT = std::unique_ptr<uint8_t>(content_pointer);

        if (packet->CRC != packp::calculate_crc(packet->CONTENT.get(), packet->CONTENT_LENGTH)){
            return packp::error_status(RECEIVE_STATUS::CRC_MISMATCH);
        }

        return std::make_pair(RECEIVE_STATUS::OK, std::shared_ptr<packp::InnerPacketV1>(packet));
    }

    //Отправка пакета
    SEND_STATUS send_packet(const std::shared_ptr<packp::InnerPacketV1>& packet, PacketManagerParams & params){
        if (packet->CONTENT_LENGTH > CONTENT_LENGTH_LIMIT_BYTES){
            return SEND_STATUS::CONTENT_LENGTH_EXCEEDED;
        }
        if (packet->CONTENT_LENGTH > 0 && packet->CONTENT == nullptr){
            return SEND_STATUS::INVALID_CONTENT;
        }

        packet->CRC = packp::calculate_crc(packet->CONTENT.get(), packet->CONTENT_LENGTH);
        auto head = reinterpret_cast<uint8_t*>(packet.get());
        auto headerLength = sizeof(packp::InnerPacketV1) - sizeof(packet->CONTENT);
        auto contentPointer = packet->CONTENT.get();

        for (auto i = 0; i < headerLength; i++)
        {
            params.write(head[i]);
        }
        for(auto i = 0; i < packet->CONTENT_LENGTH; i++){
            params.write(contentPointer[i]);
        }
    }

}
#pragma clang diagnostic pop