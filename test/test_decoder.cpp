#include <cppunit/extensions/HelperMacros.h>

#include <stdint.h>
#include <frsky/decoder.h>
#include <memory.h>

using namespace frsky;


// #define VALID_FRAME_DATE 7e fe 63 7b 53 b5 00 00 00 00 7e
// const uint8_t frame_raw_data[Decoder::FRAME_SIZE] = { 0x7e, 0xfe, 0x63, 0x7d, 0x5d, 0x61, 0xbd, 0x00, 0x00, 0x00, 0x00, 0x7e };

const uint8_t test_a1 = 0x11;
const uint8_t test_a2 = 0x12;
const uint8_t test_rssi = 0x21;
const uint8_t test_tssi = 0x22;

class DecoderTest : public CppUnit::TestFixture  {
        CPPUNIT_TEST_SUITE( DecoderTest );
        CPPUNIT_TEST( testEnsureFrameSize );
        CPPUNIT_TEST( testOutOfSync01 );
        CPPUNIT_TEST( testOutOfSync02 );
        CPPUNIT_TEST( test01 );
        CPPUNIT_TEST( testShortFrame );
//         CPPUNIT_TEST( testLostByteTwoFrames );

        CPPUNIT_TEST( testEscape01 );


        CPPUNIT_TEST_SUITE_END();

        uint8_t frame_raw_data[Decoder::FRAME_SIZE];

    public:

    void setUp() {
        static const uint8_t proto_frame_data[Decoder::FRAME_SIZE] = { 0x7e, 0xfe, test_a1, test_a2, test_rssi, test_tssi, 0x00, 0x00, 0x00, 0x00, 0x7e };

        memcpy(frame_raw_data, proto_frame_data, Decoder::FRAME_SIZE);

    }

    void testEnsureFrameSize() {
        int frame_size = Decoder::FRAME_SIZE;
        CPPUNIT_ASSERT_EQUAL(11, frame_size);
    }

    void test01() {
        Decoder d;
        CPPUNIT_ASSERT_EQUAL(0, d._idx);

        for (int i=0; i < sizeof(frame_raw_data)-1; i++) {
            CPPUNIT_ASSERT_EQUAL(Decoder::NEED_MORE_BYTES, d.process_byte(frame_raw_data[i]));
            CPPUNIT_ASSERT_EQUAL(i+1, d._idx);
        }

        CPPUNIT_ASSERT_EQUAL(Decoder::FRAME_COMPLETE_VOLTAGE_RSSI, d.process_byte(frame_raw_data[sizeof(frame_raw_data)-1]));
        CPPUNIT_ASSERT_EQUAL(0, d._idx);

        const Decoder::VOLTAGE_xSSI &xssi(d.xssi());

        CPPUNIT_ASSERT_EQUAL(test_a1, xssi.a1);
        CPPUNIT_ASSERT_EQUAL(test_a2, xssi.a2);
        CPPUNIT_ASSERT_EQUAL(test_rssi, xssi.rssi);
        CPPUNIT_ASSERT_EQUAL(test_tssi, xssi.tssi);
    }

    void testOutOfSync01() {
        Decoder d;

        CPPUNIT_ASSERT_EQUAL(Decoder::OUT_OF_SYNC, d.process_byte(Decoder::START_STOP_MARKER + 1));
        CPPUNIT_ASSERT_EQUAL(0, d._idx);
    }

    void testOutOfSync02() {
        // Invlidate frame by corrupting end marker
        frame_raw_data[Decoder::FRAME_SIZE-1] += 1;

        Decoder d;

        for (int i=0; i < sizeof(frame_raw_data)-1; i++) {
            CPPUNIT_ASSERT_EQUAL(Decoder::NEED_MORE_BYTES, d.process_byte(frame_raw_data[i]));
            CPPUNIT_ASSERT_EQUAL(i+1, d._idx);
        }

        CPPUNIT_ASSERT_EQUAL(Decoder::OUT_OF_SYNC, d.process_byte(frame_raw_data[sizeof(frame_raw_data)-1]));
        CPPUNIT_ASSERT_EQUAL(0, d._idx);

        // Feed correct frame
        frame_raw_data[Decoder::FRAME_SIZE-1] = Decoder::START_STOP_MARKER;

        for (int i=0; i < sizeof(frame_raw_data)-1; i++) {
            CPPUNIT_ASSERT_EQUAL(Decoder::NEED_MORE_BYTES, d.process_byte(frame_raw_data[i]));
            CPPUNIT_ASSERT_EQUAL(i+1, d._idx);
        }

        CPPUNIT_ASSERT_EQUAL(Decoder::FRAME_COMPLETE_VOLTAGE_RSSI, d.process_byte(frame_raw_data[sizeof(frame_raw_data)-1]));
        CPPUNIT_ASSERT_EQUAL(0, d._idx);

        const Decoder::VOLTAGE_xSSI &xssi(d.xssi());

        CPPUNIT_ASSERT_EQUAL(test_a1, xssi.a1);
        CPPUNIT_ASSERT_EQUAL(test_a2, xssi.a2);
        CPPUNIT_ASSERT_EQUAL(test_rssi, xssi.rssi);
        CPPUNIT_ASSERT_EQUAL(test_tssi, xssi.tssi);
    }


    void testShortFrame() {
        Decoder d;

        struct {
            uint8_t octet;
            Decoder::DECODE_STATE expected_state;
            int expected_idx;

        } test_data[] = {
            {Decoder::START_STOP_MARKER, Decoder::NEED_MORE_BYTES, 1},
            {1, Decoder::NEED_MORE_BYTES, 2},
            {Decoder::START_STOP_MARKER, Decoder::OUT_OF_SYNC, 1},
            {Decoder::START_STOP_MARKER, Decoder::OUT_OF_SYNC, 1},
            {1, Decoder::NEED_MORE_BYTES, 2},
            {2, Decoder::NEED_MORE_BYTES, 3},
        };


        for(int i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++)	{
            CPPUNIT_ASSERT_EQUAL(test_data[i].expected_state, d.process_byte(test_data[i].octet));
            CPPUNIT_ASSERT_EQUAL(test_data[i].expected_idx, d._idx);
        }

        uint8_t ssm = Decoder::START_STOP_MARKER;

        CPPUNIT_ASSERT_EQUAL(ssm, d._buffer[0]);
        CPPUNIT_ASSERT_EQUAL((uint8_t)1, d._buffer[1]);
        CPPUNIT_ASSERT_EQUAL((uint8_t)2, d._buffer[2]);
    }

    void testEscape01() {
        Decoder d;

        struct {
            uint8_t octet;
            Decoder::DECODE_STATE expected_state;
            int expected_idx;

        } test_data[] = {
            {Decoder::START_STOP_MARKER, Decoder::NEED_MORE_BYTES, 1},
            {1, Decoder::NEED_MORE_BYTES, 2},
            {Decoder::ESCAPE_MARKER, Decoder::NEED_MORE_BYTES, 2}, // IDX is not updated here
            {0x7E, Decoder::NEED_MORE_BYTES, 3},
            {Decoder::ESCAPE_MARKER, Decoder::NEED_MORE_BYTES, 3}, // IDX is not updated here
            {0x7D, Decoder::NEED_MORE_BYTES, 4},
            {Decoder::ESCAPE_MARKER, Decoder::NEED_MORE_BYTES, 4}, // IDX is not updated here
            {0x5E, Decoder::NEED_MORE_BYTES, 5},
            {Decoder::ESCAPE_MARKER, Decoder::NEED_MORE_BYTES, 5}, // IDX is not updated here
            {0x5D, Decoder::NEED_MORE_BYTES, 6},
        };

        for(int i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++)   {
//             printf("%d\n", i);
            CPPUNIT_ASSERT_EQUAL(test_data[i].expected_state, d.process_byte(test_data[i].octet));
            CPPUNIT_ASSERT_EQUAL(test_data[i].expected_idx, d._idx);
        }

        uint8_t ssm = Decoder::START_STOP_MARKER;

        CPPUNIT_ASSERT_EQUAL(ssm, d._buffer[0]);
        CPPUNIT_ASSERT_EQUAL((uint8_t)1, d._buffer[1]);
        CPPUNIT_ASSERT_EQUAL(0x5E, (int)d._buffer[2]);
        CPPUNIT_ASSERT_EQUAL(0x5D, (int)d._buffer[3]);
        CPPUNIT_ASSERT_EQUAL(0x7E, (int)d._buffer[4]);
        CPPUNIT_ASSERT_EQUAL(0x7D, (int)d._buffer[5]);
    }


};

CPPUNIT_TEST_SUITE_REGISTRATION( DecoderTest );
