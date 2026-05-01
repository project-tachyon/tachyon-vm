#include <Tachyon/Encoder.hpp>

void Tachyon_AMD64Encoder::RelCall(const int32_t Disp32) {
    this->Write8(0xE8);
    this->Write8(Disp32);
}

void Tachyon_AMD64Encoder::RelCall(const int16_t Disp16) {
    this->Write8(0xE8);
    this->Write8(Disp16);
}