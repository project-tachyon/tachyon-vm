#include <Tachyon/Encoder.hpp>
#include <cstdint>

// IMMEDIATE

void Tachyon_AMD64Encoder::Mov(const GpReg Dest, const uint8_t Src) {
    uint8_t Opcode = 0b10110000;
    SetModRM_Register(Opcode, Dest, true);
    this->Write8(Opcode);
    this->Write8(Src);
}

void Tachyon_AMD64Encoder::Mov(const GpReg Dest, const uint16_t Src) {
    uint8_t Opcode = 0b10111000;
    this->EmitOpsizePrefix();
    SetModRM_Register(Opcode, Dest, true);
    this->Write8(Opcode);
    this->Write16(Src);
}

void Tachyon_AMD64Encoder::Mov(const GpReg Dest, const uint32_t Src) {
    uint8_t Opcode = 0b10111000;
    SetModRM_Register(Opcode, Dest, true);
    this->Write8(Opcode);
    this->Write32(Src);
}

void Tachyon_AMD64Encoder::Mov(const GpReg Dest, const uint64_t Src) {
    uint8_t Opcode = 0b10111000;
    SetModRM_Register(Opcode, Dest, true);
    this->Write8(Opcode);
    this->Write64(Src);
}

// MEMORY ACCESS

void Tachyon_AMD64Encoder::Mov(Mem Dest, GpReg Src) {

}