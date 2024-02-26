
#include <assert.h>

#include "siyi_protocol.hpp"

static void assemble_example_message()
{
    siyi::Serializer siyi_serializer;

    siyi::GimbalCenter gimbal_center;
    const auto message = siyi_serializer.assemble_message(gimbal_center);

    // Sample from A8 mini User Manual v1.5 page 45 "Auto Centering"
    const std::vector<uint8_t> sample {0x55, 0x66, 0x01, 0x01, 0x00, 0x00, 0x00, 0x08, 0x01, 0xd1, 0x12};
    assert(message == sample);
}

static void check_sequence()
{
    siyi::Serializer siyi_serializer{};

    siyi::GimbalRotate gimbal_rotate{100, 100};
    const auto message1 = siyi_serializer.assemble_message(gimbal_rotate);

    // Sample from A8 mini User Manual v1.5 page 45 "Rotate 100, 100"
    const std::vector<uint8_t> sample1 {0x55, 0x66, 0x01, 0x02, 0x00, 0x00, 0x00, 0x07, 0x64, 0x64, 0x3d, 0xcf};
    assert(message1 == sample1);

    const auto message2 = siyi_serializer.assemble_message(gimbal_rotate);

    // Tried and it worked
    const std::vector<uint8_t> sample2 {0x55, 0x66, 0x01, 0x02, 0x00, 0x01, 0x00, 0x07, 0x64, 0x64, 0x6c, 0x65};
    assert(message2 == sample2);
}

int main(int, char**)
{
    assemble_example_message();
    check_sequence();

    return 0;
}
