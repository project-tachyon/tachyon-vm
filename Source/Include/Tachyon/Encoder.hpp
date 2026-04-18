#pragma once

#include <Tachyon/Debug.hpp>
#include <sys/mman.h>
#include <cstdint>
#include <cstddef>
#include <cstring>

typedef int (*OutputCode)(void);

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
            CodeBase = mmap(0, CodeSize, (PROT_WRITE | PROT_READ), (MAP_PRIVATE | MAP_ANON), -1, 0);
            TachyonAssert(CodeBase != MAP_FAILED);
            CodePointer = static_cast<uint8_t *>(CodeBase);
            DebugInfo("JIT compiler allocated code range: [%p - %p]\n", CodeBase, CodePointer + CodeSize);
        }
        inline OutputCode MakeExecutable(void) {
            TachyonAssert(mprotect(CodeBase, CodeSize, (PROT_EXEC | PROT_READ)) != -1);
            return (OutputCode)CodeBase;
        }

};

#ifdef __x86_64
enum class Reg : uint8_t {
    /* 8-bit low-byte registers */
    REG_AL, REG_CL, REG_DL, REG_BL, REG_SPL, REG_BPL, REG_SIL, REG_DIL,
    REG_R8B, REG_R9B, REG_R10B, REG_R11B, REG_R12B, REG_R13B, REG_R14B, REG_R15B,
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

class Tachyon_AMD64Encoder : public Tachyon_EncoderImpl {
    // https://wiki.osdev.org/X86-64_Instruction_Encoding
    private:
        inline void SetModRM_Register(uint8_t & ModRM, const Reg Register, [[gnu::unused]] const bool IsRM) {
            switch(Register) {
                case Reg::REG_AL...Reg::REG_BL: {
                    ModRM |= (static_cast<uint8_t>(Register) & 0b111) << (IsRM ? 0 : 3);
                    break;
                }
                case Reg::REG_AH...Reg::REG_BH: {
                    ModRM |= ((static_cast<uint8_t>(Register) - 12) & 0b111) << (IsRM ? 0 : 3);
                    break;
                }
                case Reg::REG_SPL...Reg::REG_DIL: {
                    TachyonUnimplemented("REX");
                    break;
                }
                case Reg::REG_EAX...Reg::REG_EDI: {
                    ModRM |= ((static_cast<uint8_t>(Register) - 36) & 0b111) << (IsRM ? 0 : 3);
                    break;
                }
                case Reg::REG_RAX...Reg::REG_RDI: {
                    this->Write8(0b01001000); // REX, 64-bit operand size
                    ModRM |= ((static_cast<uint8_t>(Register) - 52) & 0b111) << (IsRM ? 0 : 3);
                    break;
                }
                case Reg::REG_R8...Reg::REG_R15: {
                    this->Write8((0b01001000 | (IsRM ? 1 : 4))); // REX, 64-bit operand size + RM extension
                    ModRM |= ((static_cast<uint8_t>(Register) - 60) & 0b111) << (IsRM ? 0 : 3);
                    break;
                }
                default: {
                    TachyonUnimplemented("REG");
                    break;
                }
            }
        }

        inline void SetModRM_OpcodeExtension(uint8_t & ModRM, const uint8_t Opcode) {
            ModRM &= ~0x38; // REG bits
            ModRM |= (Opcode & 0b111) << 3;
        }

        inline uint8_t EncodeRR(const Reg Src, const Reg Dest) {
            // direct register access
            uint8_t ModRM = 0b11000000;
            Tachyon_AMD64Encoder::SetModRM_Register(ModRM, Src, false);
            Tachyon_AMD64Encoder::SetModRM_Register(ModRM, Dest, true);

            return ModRM;
        }
    public:
        /* naming is very sloppy */
        void MovRM_IMM8(const Reg Src, const uint8_t Imm);      // MOV RM8, IMM8
        void MovRM_IMM16(const Reg Src, const uint16_t Imm);    // MOV RM16, IMM16
        void MovRM_IMM32(const Reg Src, const uint32_t Imm);    // MOV RM32, IMM32
        void MovRM_IMM64(const Reg Src, const uint64_t Imm);    // MOV RM64, IMM64
        void MovRR_MR(const Reg Src, const Reg Dest);           // MOV R/M, REG
        void Ret(void);
};
#endif /* __x86_64 */