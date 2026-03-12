#pragma once

#include <Tachyon/Debug.hpp>
#include <Compiler.hpp>
#include <Lib/SIMDJson.h>
#include <charconv>
#include <cmath>
#include <limits>
#include <string_view>
#include <string>

using namespace simdjson;

#define IS_VALID_BASE10(c) ((c) >= '0' && (c) <= '9' || (c) == '-' || (c) == '+' || (c) == 'E' || (c) == 'e' || (c) == '.')
#define IS_INVALID_BASE10(c) ((c) == '-' || (c) == '+' || (c) == 'E' || (c) == 'e')

/* scratch uses 53-bit precision so doubles are perfect */
namespace Scratch {

    /* yes this is bigger and more sophisticated than before so i dont have to deal with manual memory management */
    /* i hate this the most out of everything in tachyon */
    typedef struct ScratchData {
        enum class Type : uint8_t { Number, String, Boolean } Type;
        union {
            std::string String;
            double Number;
            bool Boolean;
        };
        /* constructors */
        ScratchData() : Type(ScratchData::Type::Number), Number(0) {}
        ScratchData(const std::string & Value) : Type(ScratchData::Type::String) {
            new (&this->String) std::string(Value);
        }
        ScratchData(double Value) : Type(ScratchData::Type::Number), Number(Value) {}
        ScratchData(bool Value) : Type(ScratchData::Type::Boolean), Boolean(Value) {}
        /* copy constructor */
        ScratchData(const ScratchData & Other) : Type(Other.Type) {
            switch(this->Type) {
                case ScratchData::Type::String: {
                    new (&this->String) std::string(Other.String);
                    break;
                }
                case ScratchData::Type::Number: {
                    this->Number = Other.Number;
                    break;
                }
                case ScratchData::Type::Boolean: {
                    this->Boolean = Other.Boolean;
                    break;
                }
            }
        }
        /* copy assignment constructor */
        ScratchData & operator = (const ScratchData & Other) {
            if (&Other == this) {
                return *this;
            }
            if (this->Type == ScratchData::Type::String) {
                this->String.~basic_string();
            }
            this->Type = Other.Type;
            switch(this->Type) {
                case ScratchData::Type::String: {
                    new (&this->String) std::string(Other.String);
                    break;
                }
                case ScratchData::Type::Number: {
                    this->Number = Other.Number;
                    break;
                }
                case ScratchData::Type::Boolean: {
                    this->Boolean = Other.Boolean;
                    break;
                }
            }
            return *this;
        }
        /* move constructor */
        ScratchData(ScratchData && Other) noexcept : Type(Other.Type) {
            switch(this->Type) {
                case ScratchData::Type::String: {
                    new (&this->String) std::string(std::move(Other.String));
                    break;
                }
                case ScratchData::Type::Number: {
                    this->Number = Other.Number;
                    break;
                }
                case ScratchData::Type::Boolean: {
                    this->Boolean = Other.Boolean;
                    break;
                }
            }
        }
        /* move assignment constructor */
        ScratchData & operator = (ScratchData && Other) noexcept {
            if (&Other == this) {
                return *this;
            }
            if (this->Type == ScratchData::Type::String) {
                this->String.~basic_string();
            }
            this->Type = Other.Type;
            switch(this->Type) {
                case ScratchData::Type::String: {
                    new (&this->String) std::string(std::move(Other.String));
                    break;
                }
                case ScratchData::Type::Number: {
                    this->Number = Other.Number;
                    break;
                }
                case ScratchData::Type::Boolean: {
                    this->Boolean = Other.Boolean;
                    break;
                }
            }
            return *this;
        }

        friend std::ostream & operator << (std::ostream & Stream, const ScratchData & Self) {
            switch(Self.Type) {
                case ScratchData::Type::String: {
                    Stream << Self.String;
                    break;
                }
                case ScratchData::Type::Number: {
                    Stream << std::to_string(Self.Number);
                    break;
                }
                case ScratchData::Type::Boolean: {
                    Stream << (Self.Boolean == true ? "True" : "False");
                }
            }
            return Stream;
        }

        /* deconstructor */
        ~ScratchData() {
            if (this->Type == ScratchData::Type::String) {
                this->String.~basic_string();
            }
        }
    } ScratchData;
   
    typedef struct {
        std::errc ec;
        ScratchData data;
    } Snum2DataResult;

    inline Snum2DataResult __hot StringNum2ScratchData(std::string_view String) {
        if (unlikely(String.empty() == true)) {
            return { std::errc::invalid_argument, ScratchData(double(0)) };
        }
        /* remove whitespace */
        while(String[0] == ' ') String.remove_prefix(1);
        while(String[String.length() - 1] == ' ') String.remove_suffix(1);

        if (String == "Infinity" || String == "+Infinity") {
            return { std::errc(), ScratchData(std::numeric_limits<double>::infinity()) };
        } else if (String == "-Infinity") {
            return { std::errc(), ScratchData(-std::numeric_limits<double>::infinity()) };
        }
        uint8_t RadixModifier = 10;

        if (String.length() < 2) {
            goto SkipChecks;
        }

        if (String[0] == '0') {
            switch(String[1]) {
                case 'X':
                case 'x':
                    RadixModifier = 16;
                    break;
                case 'O':
                case 'o':
                    RadixModifier = 8;
                    break;
                case 'B':
                case 'b':
                    RadixModifier = 2;
                    break;
                default:
                    break;
            }
            if (RadixModifier != 10) {
                String.remove_prefix(2);
                uint64_t NonDecConversion;
                std::from_chars_result Result = std::from_chars(String.begin(), String.end(), NonDecConversion, RadixModifier);
                if (Result.ec == std::errc::invalid_argument) {
                    /* bad num */
                    return { std::errc::invalid_argument, ScratchData(double(0)) };
                } else if (Result.ec == std::errc::result_out_of_range) {
                    /* other possible result could be out of range (infinity for scratch) */
                    return { std::errc(), ScratchData(std::numeric_limits<double>::infinity()) };
                }
                return { std::errc(), ScratchData(double(NonDecConversion)) };
            }
        }
SkipChecks:
        if (IS_INVALID_BASE10(String[String.length() - 1]) == true) {
            return { std::errc::invalid_argument, ScratchData(double(0)) };
        }
        bool PastEuler = false;
        for(size_t i = 0; i < String.length(); i++) {
            const char c = String[i];
            if (IS_VALID_BASE10(c) == false) {
                return { std::errc::invalid_argument, ScratchData(double(0)) };
            }
            if (PastEuler) {
                if (c == '.') {
                    return { std::errc::invalid_argument, ScratchData(double(0)) };
                }
            }
            if (c == 'e' || c == 'E') {
                PastEuler = true;
            }
        }
        double ConvertedBase10;
        std::from_chars_result Result = std::from_chars(String.begin(), String.end(), ConvertedBase10);
        if (Result.ec == std::errc::invalid_argument) {
            return { std::errc::invalid_argument, ScratchData(double(0)) };
        } else if (Result.ec == std::errc::result_out_of_range) {
            return { std::errc(), ScratchData((String[0] == '-') ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity()) };
        }
        /* could be nan or infinity */
        if (unlikely(std::isnan(ConvertedBase10) == true)) {
            return { std::errc(), ScratchData("NaN") };
        }
        return { std::errc(), ScratchData(ConvertedBase10) };
    }

    inline const std::string __hot Data2String(ScratchData & Data) {
        if (Data.Type == ScratchData::Type::String) {
            return Data.String;
        } else if (Data.Type == ScratchData::Type::Number) {
            /* TODO: set to low precision somehow */
            return std::to_string(Data.Number);
        } else {
            return (Data.Boolean == true ? "true" : "false");
        }
        __unreachable;
    }

    inline bool __hot StringIsNumber(std::string_view String) {
        Snum2DataResult Result = StringNum2ScratchData(String);
        if (Result.ec == std::errc::invalid_argument) {
            return false;
        }
        return true;
    }

    inline ScratchData __hot SanitizeData(ondemand::value VariableData) {
        switch(VariableData.type()) {
            case ondemand::json_type::string: {
                /* not too reliable to detect strings. could be hex, octal, binary, or a number. */
                std::string SanitizedString(VariableData.get_string().value());
                if (StringIsNumber(SanitizedString) == true) {
                    Snum2DataResult Result = StringNum2ScratchData(SanitizedString);
                    if (Result.ec == std::errc::invalid_argument) {
                        /* normal string, not a number */
                        return ScratchData(SanitizedString);
                    }
                    /* conversion success */
                    return Result.data;
                }
                /* normal string */
                return ScratchData(SanitizedString);
            }
            case ondemand::json_type::boolean: {
                bool SanitizedBool = VariableData.get_bool().value();
                return ScratchData(SanitizedBool);
            }
            case ondemand::json_type::number: {
                /* various number types */
                switch(VariableData.get_number_type()) {
                    case ondemand::number_type::big_integer: {
                        return ScratchData(std::numeric_limits<double>::infinity());
                    }
                    case ondemand::number_type::floating_point_number: {
                        double SanitizedDouble = VariableData.get_double().value();
                        return ScratchData(SanitizedDouble);
                    }
                    default: {
                        double SanitizedNum = VariableData.get_number()->as_double();
                        return ScratchData(SanitizedNum);
                    }
                }
            }
            default: {
                return ScratchData(double(0));
            }
        }
    }

    class ScratchVariable_Base {
        public:
            inline std::string GetName(void) {
                return this->Name;
            }
            inline bool IsPublic(void) {
                return this->Public;
            }
        protected:
            std::string Name;
            bool Public;
    };

    class ScratchVariable : ScratchVariable_Base {
        public:
            ScratchVariable(ondemand::array VariableData, const bool IsPublic) {
                /* VariableData[0] = list name, VariableData[1] = actual data */
                this->Name = std::string(VariableData.at(0).get_string().value());
                VariableData.reset();
                this->Data = SanitizeData(VariableData.at(1).value());
                VariableData.reset();
                this->Public = IsPublic;
            }

            inline void __hot SetData(ScratchData & NewData) {
                this->Data = NewData;
            }
            inline const ScratchData __hot GetData(void) {
                return Data;
            }
        private:
            ScratchData Data;
    };

    class ScratchList : ScratchVariable_Base {
        public:
            ScratchList (ondemand::array ListData, const bool IsPublic) {
                /* same applies for ScratchList as it does for ScratchVariable; 
                 * ListData[0] = list name, ListData[1] = [actual data] */
                this->Name = std::string(ListData.at(0)->get_string().value());
                ListData.reset();
                this->Public = IsPublic;
                /* prepare for lazy loading */
                DataJson = ListData.at(1).get_array().value();
                this->TotalItems = DataJson.count_elements();
                if (this->TotalItems >= 200000) {
                    DebugWarn("List \"%s\" goes over 200,000 items; memory usage is bound to increase.\n", this->Name.c_str());
                    DebugInfo("To reduce memory usage, consider using psuedo-blocks to create a less memory-expensive buffer\n");
                }
                /* we dont load it. we lazily load things on-demand */
            }

            inline ScratchData __hot Get(size_t ItemIndex) {
                if (ItemIndex >= this->TotalItems) {
                    return ScratchData("");
                }
                return ScratchData("haha");
            }
            size_t TotalItems = 0;
        private:
            padded_string ListJson;
            ondemand::array DataJson;
            std::vector<ScratchData> Elements;
    };

    namespace Data {
        void RegisterAll(void);
    };
};
