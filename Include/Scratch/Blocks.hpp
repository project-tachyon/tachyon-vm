#pragma once

#include <Scratch/BlockFields.hpp>
#include <Scratch/Data.hpp>
#include <Lib/SIMDJson.h>
#include <Compiler.hpp>
#include <string>

using namespace simdjson;

/* input shadow types */
#define INPUT_IS_SHADOW               1
#define INPUT_NO_SHADOW               2
#define INPUT_REPORTER_BLOCK          3
/* primitives */
#define INPUT_PRIMITIVE_MATH_NUM      4
#define INPUT_PRIMITIVE_POSITIVE_NUM  5
#define INPUT_PRIMITIVE_WHOLE_NUM     6
#define INPUT_PRIMITIVE_INTEGER_NUM   7
#define INPUT_PRIMITIVE_ANGLE_NUM     8
#define INPUT_PRIMITIVE_COLOR_PICKER  9
#define INPUT_PRIMITIVE_TEXT          10
#define INPUT_PRIMITIVE_BROADCAST     11
#define INPUT_PRIMITIVE_VAR           12
#define INPUT_PRIMITIVE_LIST          13

namespace Scratch {
    class ScratchSprite;

    /**
     * Contains status codes returned by opcode handlers.
     */
    enum class ScratchStatus : uint8_t {
        SCRATCH_END,
        SCRATCH_NEXT,
        SCRATCH_YIELD_WAIT,
        SCRATCH_YIELD_WAIT_UNTIL
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
            ScratchBlock (std::string Key, ondemand::object BlockData, ScratchSprite & Owner) : Sprite(Owner) {
                /* basic info */
                this->Opcode = std::string(BlockData["opcode"]->get_string().value());
                this->TopLevel = BlockData["topLevel"]->get_bool().value();
                this->Shadow = BlockData["shadow"]->get_bool().value();
                this->ProcedureDefinition = (this->Opcode == "procedures_definition");
                this->BlockKey = Key;
                if (BlockData["next"].is_null() == false) {
                    this->NextBlock_Key = std::string(BlockData["next"]->get_string().value());
                }
                if (BlockData["parent"].is_null() == false) {
                    this->ParentBlock_Key = std::string(BlockData["parent"]->get_string().value());
                }
                /* mutation */
                auto FindResult = BlockData.find_field("mutation");
                if (FindResult.error() == error_code::SUCCESS) {
                    Mutation = this->ParseMutation(BlockData["mutation"]);
                }
                /* inputs */
                try {
                    ondemand::object InputData = BlockData["inputs"]->get_object().value();
                    for (ondemand::field InputField : InputData) {
                        std::string InputKey = std::string(InputField.unescaped_key().value());
                        Inputs.emplace_back(this->ParseInput(InputKey, InputField.value()));
                    }
                } catch (simdjson_error & Error) {
                    std::cerr << Error.what() << std::endl;
                }
                /* assign function based on opcode */
                this->LinkHandlers();
            }

            ~ScratchBlock(void) {
                this->Inputs.clear();
            }

            /**
             * Gets the block's opcode.
             * @returns The block's opcode.
             */
            inline std::string & GetOpcode(void) {
                return this->Opcode;
            }

            /**
             * Gets the next block's key.
             * @returns The next block's key.
             */
            inline std::string & GetNextKey(void) {
                return this->NextBlock_Key;
            }

            /**
             * Gets the parent block's key.
             * @returns The parent block's key.
             */
            inline std::string & GetParentKey(void) {
                return this->ParentBlock_Key;
            }

            /**
             * Gets the parent block's key.
             * @returns The parent block's key.
             */
            inline std::string & GetKey(void) {
                return this->BlockKey;
            }

            /**
             * Checks if the block is a procedure definition.
             * @returns True if it's a procedure definition, false if otherwise.
             */
            inline bool IsProcedureDef(void) {
                return this->ProcedureDefinition;
            }

            /**
             * Checks if the block is an argument reporter.
             * @returns True if it's an argument reporter, false if otherwise.
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
                    return { .Type = ScratchData::Type::Number, .Number = 0 };
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
             * @returns The block's sprite.
             */
            ScratchSprite & __hot GetOwnerSprite(void);

            ScratchData __hot GetInputData(size_t InputNum);
            ScratchInput __hot GetInput(size_t InputNum);
        private:
            /**
             * Should only be used for non-reporter blocks.
             */
            OpcodeHandler Handler = nullptr;

            /**
             * Should only be used for reporter blocks.
             */
            EvaluationHandler ReporterHandler = nullptr;

            /**
             * Links an opcode handler to the current block.
             */
            void LinkHandlers(void);

            ScratchMutation ParseMutation(ondemand::object MutationObject);
            ScratchInput ParseInput(std::string & Key, ondemand::array InputObject);
            ScratchField ParseField(std::string & Key, ondemand::array FieldObject);

            std::vector<ScratchInput> Inputs;
            std::vector<ScratchField> Fields;
            std::string Opcode;
            std::string NextBlock_Key;
            std::string ParentBlock_Key;
            std::string BlockKey;
            std::reference_wrapper<ScratchSprite> Sprite;
            std::optional<ScratchMutation> Mutation;
            bool Shadow;
            bool TopLevel;
            bool ProcedureDefinition;
            bool ArgumentReporter;
    };
};
