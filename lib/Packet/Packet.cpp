#include <Packet.h>
#include <vector>
#include <packet_extensions.cpp>

#pragma region LowLevel

void inner_send_packet(std::shared_ptr<InnerPacket> packet, std::function<void(uint8_t)> write){
    auto head = reinterpret_cast<uint8_t*>(packet.get());
    auto headerLength = sizeof(InnerPacket) - sizeof(packet->CRC) - sizeof(packet->CONTENT);

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

std::pair<RECIEVE_STATUS, std::shared_ptr<InnerPacket>> inner_recieve_packet(std::function<uint8_t(void)> read, std::function<int(void)> available, short polling_delay_ms = 50, unsigned long timeout = 0){

    unsigned long polling_start_time = millis();
    
    //Собираемый пакет
    auto packet = new InnerPacket();

    inline auto timeout_reached = [&](){
        return timeout != 0 && millis() - polling_start_time > timeout;
    };

    inline auto read_n_bytes = [&](uint8_t* pointer, size_t n){
        auto remaining = n;
        while(remaining > 0){
            if(timeout_reached()){
                return RECIEVE_STATUS::TIMEOUT_REACHED;
            }
            if(available() > 0){
                pointer[0] = read();
                pointer++;
                remaining--;
            }
            delay(polling_delay_ms);
        }
        return RECIEVE_STATUS::OK;
    };


    std::vector<uint8_t> buffer;
    while(true){
        if (timeout_reached()){
            return std::make_pair(RECIEVE_STATUS::TIMEOUT_REACHED, std::make_shared<InnerPacket>(nullptr));
        }

        auto available_count = available();
        
        if(buffer.size() < sizeof(MAGICHEADER)){
            buffer.push_back(read());
        }
        if (buffer == packet::MAGIC_BYTES){
            buffer.clear();
            break;
        }else{ // Если байты в буфере не совпали - удаляем первый, готовимся считать очередной
            buffer.erase(buffer.begin());
        }

        delay(polling_delay_ms);
    }
    
    //Чтение заголовка
    auto header_pointer = reinterpret_cast<uint8_t*>(packet) + sizeof(InnerPacket::MAGIC);
    auto hl_without_magic = sizeof(InnerPacket) - sizeof(InnerPacket::CONTENT) - sizeof(InnerPacket::MAGIC);

    auto header_reading_status = read_n_bytes(header_pointer, hl_without_magic);

    if (header_reading_status != RECIEVE_STATUS::OK){
        return packet::make_error_status(header_reading_status);
    }

    if (packet->CONTENT_LENGTH < 1 || packet->CONTENT_LENGTH > CONTENT_LENGTH_LIMIT_BYTES){
        return packet::make_error_status(RECIEVE_STATUS::CONTENT_LENGTH_EXCEEDED);
    }

    //Чтение содержимого
    auto content_pointer = new uint8_t[packet->CONTENT_LENGTH];
    auto content_reading_status = read_n_bytes(content_pointer, packet->CONTENT_LENGTH);
    if (content_reading_status != RECIEVE_STATUS::OK){
        return packet::make_error_status(content_reading_status);
    }

    packet->CONTENT = std::unique_ptr<uint8_t>(content_pointer);
    
    if (packet->CRC != packet::calculate_crc(packet->CONTENT.get(), packet->CONTENT_LENGTH)){
        return packet::make_error_status(RECIEVE_STATUS::CRC_MISMATCH);
    }

    return std::make_pair(RECIEVE_STATUS::OK, std::make_shared<InnerPacket>(packet));
}

#pragma endregion

#pragma region PacketManager

SEND_STATUS PacketManager::send(std::vector<uint8_t> data){
    
}

#pragma endregion