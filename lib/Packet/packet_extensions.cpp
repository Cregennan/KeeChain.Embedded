#include <Packet.h>
#include <vector>

namespace packet{
    inline uint8_t magic_nth_byte_msb(int n){
        return MAGICHEADER >> (8 * (sizeof(MAGICHEADER) - n) & 0xFF);
    };

    static const auto MAGIC_BYTES = value_to_vector_msb(MAGICHEADER);

    template <typename T>
    inline std::vector<uint8_t> value_to_vector_msb(const T& value) {
        size_t size = sizeof(value);
        std::vector<uint8_t> byte_vector(size);
        for (size_t i = 0; i < size; ++i) {
            byte_vector[size - 1 - i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
        }

        return byte_vector;
    }

    inline std::pair<RECIEVE_STATUS, std::shared_ptr<InnerPacket>> make_error_status(RECIEVE_STATUS status){
        return std::make_pair(status, std::shared_ptr<InnerPacket>(nullptr));
    }

    inline decltype(InnerPacket::CRC) calculate_crc(uint8_t* pointer, size_t n){
        decltype(InnerPacket::CRC) crc = 0;
        for (size_t i = 0; i < n; i++)
        {
            crc ^= pointer[i] << (i % sizeof(InnerPacket::CRC));
        }

        return crc;
    }

    inline bool 
}