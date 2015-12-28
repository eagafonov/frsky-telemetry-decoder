#ifndef _FRSKY_TELEMETRY_DECODER_H_
#define _FRSKY_TELEMETRY_DECODER_H_

#include <stdint.h>

class DecoderTest;

namespace frsky {
    class Decoder {
    public:
        enum DECODE_STATE {
            NEED_MORE_BYTES = 0,
            FRAME_COMPLETE_VOLTAGE_RSSI,
            FRAME_COMPLETE,
            OUT_OF_SYNC,
        };

        typedef struct {
            uint8_t a1;
            uint8_t a2;
            uint8_t rssi;
            uint8_t tssi;
        } VOLTAGE_xSSI;

        static const uint8_t START_STOP_MARKER = 0x7E;
        static const uint8_t ESCAPE_MARKER = 0x7D;
        static const uint8_t ESCAPE_XOR_MASK = 0x20;
        static const int FRAME_SIZE = 11;

        Decoder();
        DECODE_STATE process_byte(uint8_t incoming_ocetet);

        int voltage_frame_counter() {
            return _voltage_frame_counter;
        };

        VOLTAGE_xSSI& xssi() {
            return _xssi;
        }

    protected:
        uint8_t _buffer[FRAME_SIZE];
        int _idx;
        bool _escape;

        int _voltage_frame_counter;

        VOLTAGE_xSSI _xssi;

        void reset() {
            _idx = 0;
            _escape = false;
        }

        bool is_full() {
            return _idx == sizeof(_buffer);
        };

        DECODE_STATE decode();

        friend class ::DecoderTest;
    };
};
#endif
