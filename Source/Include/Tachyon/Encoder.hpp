#pragma once

#include <Tachyon/Debug.hpp>
#include <Tachyon/ExMem.hpp>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <variant>

typedef int (*OutputCode)(void);

struct Tachyon_VMState {
    uint64_t rbx, rsp, rbp;
    uint64_t r12, r13, r14, r15;
};

class Tachyon_EncoderImpl {
    protected:
        void * CodeBase;
        uint8_t * CodePointer;
        size_t CodeSize;

        void Write8(uint8_t Byte) {
            *CodePointer++ = Byte;
        }
        void Write16(uint16_t Word) {
            memcpy(CodePointer, &Word, 2);
            CodePointer += 2;
        }
        void Write32(uint32_t Dword) {
            memcpy(CodePointer, &Dword, 4);
            CodePointer += 4;
        }
        void Write64(uint64_t Qword) {
            memcpy(CodePointer, &Qword, 8);
            CodePointer += 8;
        }
    public:
        Tachyon_EncoderImpl() {
            CodeSize = 4096;
            CodeBase = Tachyon::AllocateCodeMemory(CodeSize);
            TachyonAssert(CodeBase != nullptr);
            CodePointer = static_cast<uint8_t *>(CodeBase);
            DebugInfo("JIT compiler allocated code range: [%p - %p]\n", CodeBase, CodePointer + CodeSize);
        }

        inline OutputCode MakeExecutable(void) {
            TachyonAssert(Tachyon::ProtectCodeMemory(CodeBase, CodeSize) == true);
            return (OutputCode)CodeBase;
        }

};

#if defined(__x86_64__) || defined(_M_AMD64)
class GpReg {
    public:
        enum RegisterKind : uint8_t {
            /* 8-bit low-byte registers */
            REG_AL, REG_CL, REG_DL, REG_BL, REG_SPL, REG_BPL, REG_SIL, REG_DIL,
            REG_R8L, REG_R9L, REG_R10L, REG_R11L, REG_R12L, REG_R13L, REG_R14L, REG_R15L,
            /* 8-bit high-byte registers */
            REG_AH, REG_CH, REG_DH, REG_BH,
            /* 16-bit registers */
            REG_AX, REG_CX, REG_DX, REG_BX, REG_SP, REG_BP, REG_SI, REG_DI,
            REG_R8W, REG_R9W, REG_R10W, REG_R11W, REG_R12W, REG_R13W, REG_R14W, REG_R15W,
            /* 32-bit registers */
            REG_EAX, REG_ECX, REG_EDX, REG_EBX, REG_ESP, REG_EBP, REG_ESI, REG_EDI,
            REG_R8D, REG_R9D, REG_R10D, REG_R11D, REG_R12D, REG_R13D, REG_R14D, REG_R15D,
            /* 64-bit registers */
            REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP, REG_RSI, REG_RDI,
            REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15,
        };

        GpReg() = default;
        constexpr GpReg(const GpReg::RegisterKind Reg) : Value(Reg) {}
        explicit operator bool() const = delete;

        constexpr operator GpReg::RegisterKind() const {
            return this->Value;
        }

        constexpr bool operator == (const GpReg::RegisterKind Reg) const {
            return this->Value == Reg;
        }

        constexpr bool operator != (const GpReg::RegisterKind Reg) const {
            return this->Value != Reg;
        }

        constexpr uint8_t AsRegID(void) {
            switch(this->Value) {
                case GpReg::REG_AL...GpReg::REG_BL: {
                    return this->Value & 0b111;
                }
                case GpReg::REG_AH...GpReg::REG_BH: {
                    return (this->Value - GpReg::REG_AH) & 0b111;
                }
                case GpReg::REG_SPL...GpReg::REG_DIL: {
                    return this->Value & 0b111;
                }
                case GpReg::REG_R8L...GpReg::REG_R15L: {
                    return (this->Value - GpReg::REG_R8L) & 0b111;
                }
                /* 16-bit */
                case GpReg::REG_AX...GpReg::REG_DI: {
                    return (this->Value - GpReg::REG_AX) & 0b111;
                }
                case GpReg::REG_R8W...GpReg::REG_R15W: {
                    return (this->Value - GpReg::REG_R8W) & 0b111;
                }
                /* 32-bit */
                case GpReg::REG_EAX...GpReg::REG_EDI: {
                    return (this->Value - GpReg::REG_EAX) & 0b111;
                }
                case GpReg::REG_R8D...GpReg::REG_R15D: {
                    return (this->Value - GpReg::REG_R8D) & 0b111;
                }
                /* 64-bit */
                case GpReg::REG_RAX...GpReg::REG_RDI: {
                    return ((this->Value) - GpReg::REG_RAX) & 0b111;
                }
                case GpReg::REG_R8...GpReg::REG_R15: {
                    return (this->Value - GpReg::REG_R8) & 0b111;
                }
                default: {
                    return 0;
                }
            }
        }
    private:
        RegisterKind Value;
};

class Mem {
    public:
        struct Sib {
            uint8_t Scale;
            GpReg Index;
            GpReg Base;
            uint32_t Disp;
        };
        Mem(const uint8_t S, const GpReg I, const GpReg B) {
            this->Value = (struct Sib) {
                S,
                I,
                B,
                0
            };
        }
        Mem(const uint8_t S, const GpReg I, const GpReg B, uint32_t Displacement) {
            this->Value = (struct Sib) {
                S,
                I,
                B,
                Displacement
            };
        }
        Mem(void * Addr) : Value(Addr) {}
        constexpr bool IsSib(void) {
            return std::holds_alternative<struct Sib>(Value);
        }
    private:
        std::variant<struct Sib, void *> Value;
};

class Tachyon_AMD64Encoder : public Tachyon_EncoderImpl {
    // https://wiki.osdev.org/X86-64_Instruction_Encoding
    private:
        inline void EmitREX(const bool Opsize64, const bool R, const bool X, const bool B) {
            uint8_t REX = (0x40 | (Opsize64 << 3) | (R << 2) | (X << 1) | B);
            this->Write8(REX);
        }

        // op-size prefix (32 -> 16)
        inline void EmitOpsizePrefix(void) {
            this->Write8(0x66);
        }

        inline void SetREG(uint8_t & ModRM, const uint8_t Reg) {
            ModRM &= ~0x38; // REG bits
            ModRM |= (Reg & 0b111) << 3;
        }

        inline void SetRM(uint8_t & ModRM, const uint8_t RM) {
            ModRM &= ~0x7; // RM bits
            ModRM |= (RM & 0b111);
        }

        inline void SetModRM_Register(uint8_t & ModRM, GpReg Register, const bool IsRM) {
            switch(Register) {
                case GpReg::REG_SPL...GpReg::REG_DIL: {
                    this->EmitREX(0, not IsRM, 0, IsRM);
                    ModRM |= Register.AsRegID() << (IsRM ? 0 : 3);
                    break;
                }
                case GpReg::REG_R8L...GpReg::REG_R15L:
                case GpReg::REG_R8W...GpReg::REG_R15W:
                case GpReg::REG_R8D...GpReg::REG_R15D: {
                    this->EmitREX(0, not IsRM, 0, IsRM);
                    ModRM |= Register.AsRegID() << (IsRM ? 0 : 3);
                    break;
                }
                case GpReg::REG_R8...GpReg::REG_R15: {
                    this->EmitREX(1, not IsRM, 0, IsRM);
                    ModRM |= Register.AsRegID() << (IsRM ? 0 : 3);
                    break;
                }
                case GpReg::REG_RAX...GpReg::REG_RDI: {
                    this->EmitREX(1, not IsRM, 0, IsRM);
                    ModRM |= Register.AsRegID() << (IsRM ? 0 : 3);
                    break;
                }
                /* non REX */
                default: {
                    ModRM |= Register.AsRegID() << (IsRM ? 0 : 3);
                    break;
                }
            }
        }

        inline void EmitModRM(uint8_t Mod, uint8_t Reg, uint8_t RM) {
            uint8_t ModRM = (Mod & 3) << 6;
            ModRM |= (Reg & 7) << 3;
            ModRM |= (RM & 7);
            this->Write8(ModRM);
        }
    public:
        void EmitTachyonPrologue(const Tachyon_VMState & State);
        void EmitTachyonEpilogue(void);

        void Mov(const GpReg Dest, const uint8_t Imm);
        void Mov(const GpReg Dest, const uint16_t Imm);
        void Mov(const GpReg Dest, const uint32_t Imm);
        void Mov(const GpReg Dest, const uint64_t Imm);

        void Mov(const Mem Dest, const GpReg Src);

        void RelCall(const int32_t Disp32);
        void RelCall(const int16_t Disp16);

        void Ret(void);
        void Ret(uint16_t Bytes);
};
#endif /* __x86_64 */