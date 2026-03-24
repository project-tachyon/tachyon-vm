#pragma once

#include "Tachyon/Debug.hpp"
#include <Scratch/BlockFields.hpp>
#include <Scratch/Data.hpp>
#include <Lib/SIMDJson.h>
#include <Compiler.hpp>
#include <string>

using namespace simdjson;

namespace Scratch {
    class ScratchSprite;

    /**
     * Contains status codes returned by opcode handlers.
     */
    enum class ScratchStatus : uint8_t {
        SCRATCH_END,
        SCRATCH_NEXT,
        SCRATCH_PAUSE,
        SCRATCH_WAIT,
        SCRATCH_WAIT_UNTIL
    };

    class ScratchBlock;

    /**
     * Contains block mutation information.
     */
    struct ScratchMutation {
        std::vector<std::string> ParametersKeys;
        std::vector<std::string> ParametersNames;
        std::vector<ScratchData> ParameterDefaults;
        std::string ProcCode;
        bool HasNext;
        bool UseWarp;
    };
    

    /**
     * Scratch opcode function handler.
     */

    using OpcodeHandler = ScratchStatus (*)(ScratchBlock &);
    using EvaluationHandler = ScratchData (*)(ScratchBlock &);

    /**
     * Contains scratch block information.
     */
    class ScratchBlock {
        public:
            /**
             * Scratch block constructor.
             * @param The block's key ID.
             * @param The block's JSON data.
             */
            ScratchBlock (std::string Key, ondemand::object BlockData, ScratchSprite & Owner) : Sprite(Owner), BlockKey(Key) {
                /*
                    opcode
                */
                simdjson::simdjson_result Result = BlockData.find_field_unordered("opcode");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_string().get(this->Opcode) == error_code::SUCCESS);
                /*
                    topLevel
                */
                Result = BlockData.find_field_unordered("topLevel");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_bool().get(this->TopLevel) == error_code::SUCCESS);
                /*
                    shadow
                */
                Result = BlockData.find_field_unordered("shadow");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_bool().get(this->Shadow) == error_code::SUCCESS);

                this->ProcedureDefinition = (this->Opcode == "procedures_definition");
                this->ProcedurePrototype = (this->Opcode == "procedures_prototype");
                this->ProcedureCall = (this->Opcode == "procedures_call");

                /*
                    next
                */
                Result = BlockData.find_field_unordered("next");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                bool IsNull;
                TachyonAssert(Result.is_null().get(IsNull) == error_code::SUCCESS);
                if (IsNull == false) {
                    TachyonAssert(Result.get_string().get(this->NextBlock_Key) == error_code::SUCCESS);
                }
                /*
                    parent
                */
                Result = BlockData.find_field_unordered("parent");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.is_null().get(IsNull) == error_code::SUCCESS);
                if (IsNull == false) {
                    TachyonAssert(Result.get_string().get(this->ParentBlock_Key) == error_code::SUCCESS);
                }
                /*
                    mutation
                */
                Result = BlockData.find_field_unordered("mutation");
                if (Result.error() == error_code::SUCCESS) {
                    ondemand::object MutationObject;
                    TachyonAssert(Result.get_object().get(MutationObject) == error_code::SUCCESS);
                    Mutation = this->ParseMutation(MutationObject);
                }
                /* 
                    inputs 
                */
                Result = BlockData.find_field_unordered("inputs");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                ondemand::object InputData;
                TachyonAssert(Result.get_object().get(InputData) == error_code::SUCCESS);
                for (auto InputField : InputData) {
                    std::string InputKey;
                    TachyonAssert(InputField.unescaped_key(InputKey) == error_code::SUCCESS);

                    ondemand::array InputArray;
                    TachyonAssert(InputField.value().get_array().get(InputArray) == error_code::SUCCESS);

                    this->Inputs.emplace_back(
                        this->ParseInput(InputKey, InputArray)
                    );
                }
                if (unlikely(this->Inputs.empty() == false)) {
                    /* sort inputs */
                    std::sort(this->Inputs.begin(), this->Inputs.end(), [](const ScratchInput & A, const ScratchInput & B) {
                        return A.Type < B.Type;
                    });
                }
                /*
                    fields
                */
                Result = BlockData.find_field_unordered("fields");
                TachyonAssert(Result.error() == error_code::SUCCESS);
                ondemand::object FieldData;
                TachyonAssert(Result.get_object().get(FieldData) == error_code::SUCCESS);
                for (auto FieldField : FieldData) {
                    std::string FieldKey;
                    TachyonAssert(FieldField.unescaped_key(FieldKey) == error_code::SUCCESS);

                    ondemand::array FieldArray;
                    TachyonAssert(FieldField.value().get_array().get(FieldArray) == error_code::SUCCESS);

                    this->Fields.emplace_back(
                        this->ParseField(FieldKey, FieldArray)
                    );
                }
                /* assign function based on opcode */
                this->LinkHandlers();
            }

            ~ScratchBlock(void) {
                this->Inputs.clear();
            }

            /**
             * Gets the block's opcode.
             * @return The block's opcode.
             */
            inline std::string & GetOpcode(void) {
                return this->Opcode;
            }

            /**
             * Gets the next block's key.
             * @return The next block's key.
             */
            inline std::string & GetNextKey(void) {
                return this->NextBlock_Key;
            }

            /**
             * Gets the parent block's key.
             * @return The parent block's key.
             */
            inline std::string & GetParentKey(void) {
                return this->ParentBlock_Key;
            }

            /**
             * Gets the parent block's key.
             * @return The parent block's key.
             */
            inline std::string & GetKey(void) {
                return this->BlockKey;
            }

            /**
             * Checks if the block is a procedure definition.
             * @return True if it's a procedure definition, false if otherwise.
             */
            inline bool IsProcedureDef(void) {
                return this->ProcedureDefinition;
            }

            /**
             * Checks if the block is a procedure definition.
             * @return True if it's a procedure definition, false if otherwise.
             */
            inline bool IsProcedurePrototype(void) {
                return this->ProcedurePrototype;
            }

            /**
             * Checks if the block is a procedure definition.
             * @return True if it's a procedure definition, false if otherwise.
             */
            inline bool IsProcedureCall(void) {
                return this->ProcedureCall;
            }

            /**
             * Checks if the block is an argument reporter.
             * @return True if it's an argument reporter, false if otherwise.
             */
            inline bool IsArgumentReporter(void) {
                return this->ArgumentReporter;
            }
            
            /**
             * Executes the current block.
             */
            inline ScratchStatus __hot Execute(void) {
                if (unlikely(this->Handler == nullptr)) {
                    std::cerr << "WARNING: Invalid opcode: " << this->Opcode << std::endl;
                    return ScratchStatus::SCRATCH_END;
                }
                return this->Handler(*this);
            }

            /**
             * Executes and returns the block's result.
             */
            inline ScratchData __hot Evaluate(void) {
                if (unlikely(this->ReporterHandler == nullptr)) {
                    std::cerr << "WARNING: No reporter handler registered for " << this->Opcode << std::endl;
                    return ScratchData(double(0));
                }
                return this->ReporterHandler(*this);
            }

            /**
             * Gets the block's mutation (if it exists).
             */
            inline ScratchMutation & __hot GetMutation(void) {
                return this->Mutation.value();
            }

            /**
             * Get's the block's sprite.
             * @return The block's sprite.
             */
            ScratchSprite & __hot GetOwnerSprite(void);

            ScratchData __hot GetInputData(size_t InputNum);
            ScratchInput __hot GetInput(size_t InputNum);
            ScratchField __hot GetField(size_t FieldNum);
        private:
            std::optional<ScratchMutation> Mutation;

            /**
             * Links an opcode handler to the current block.
             */
            void LinkHandlers(void);

            ScratchMutation ParseMutation(ondemand::object MutationObject);
            ScratchInput ParseInput(std::string & Key, ondemand::array InputObject);
            ScratchField ParseField(std::string & Key, ondemand::array FieldObject);

            std::string Opcode;
            std::string NextBlock_Key;
            std::string ParentBlock_Key;
            std::string BlockKey;

            std::vector<ScratchInput> Inputs;
            std::vector<ScratchField> Fields;
            std::reference_wrapper<ScratchSprite> Sprite;

            /**
             * Should only be used for non-reporter blocks.
             */
            OpcodeHandler Handler = nullptr;

            /**
             * Should only be used for reporter blocks.
             */
            EvaluationHandler ReporterHandler = nullptr;

            bool Shadow;
            bool TopLevel;
            bool ProcedureDefinition;
            bool ProcedurePrototype;
            bool ProcedureCall;
            bool ArgumentReporter;
    };
};
