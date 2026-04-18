#include <Tachyon/Encoder.hpp>

/*
    TODO: Function overloading
*/

void Tachyon_AMD64Encoder::MovRR_MR(const Reg Src, const Reg Dest) {
    /* writes REX byte if needed */
    uint8_t ModRM = Tachyon_AMD64Encoder::EncodeRR(Src, Dest);
    this->Write8(ModRM);
}

void Tachyon_AMD64Encoder::MovRM_IMM8(const Reg Dest, const uint8_t Src) {
    uint8_t Opcode = 0b10110000;
    SetModRM_Register(Opcode, Dest, true);
    this->Write8(Opcode);
    this->Write8(Src);
}

void Tachyon_AMD64Encoder::MovRM_IMM16(const Reg Dest, const uint16_t Src) {
    uint8_t Opcode = 0b10111000;
    this->Write8(0x66); // op-size prefix (32 -> 16)
    SetModRM_Register(Opcode, Dest, true);
    this->Write8(Opcode);
    this->Write16(Src);
}

void Tachyon_AMD64Encoder::MovRM_IMM32(const Reg Dest, const uint32_t Src) {
    uint8_t Opcode = 0b10111000;
    SetModRM_Register(Opcode, Dest, true);
    this->Write8(Opcode);
    this->Write32(Src);
}

void Tachyon_AMD64Encoder::MovRM_IMM64(const Reg Dest, const uint64_t Src) {
    uint8_t Opcode = 0b10111000;
    SetModRM_Register(Opcode, Dest, true);
    this->Write8(Opcode);
    this->Write64(Src);
}

void Tachyon_AMD64Encoder::Ret(void) {
    this->Write8(0xC3);
}