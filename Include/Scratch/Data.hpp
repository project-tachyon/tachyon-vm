#pragma once

#include <Compiler.hpp>
#include <Lib/SIMDJson.h>
#include <charconv>
#include <cmath>
#include <limits>
#include <string_view>
#include <system_error>
#include <string>

using namespace simdjson;

#define IS_VALID_BASE10(c) ((c) >= '0' && (c) <= '9' || (c) == '-' || (c) == '+' || (c) == 'E' || (c) == 'e' || (c) == '.')
#define IS_INVALID_BASE10(c) ((c) == '-' || (c) == '+' || (c) == 'E' || (c) == 'e')

/* scratch uses 53-bit precision so doubles are perfect */
namespace Scratch {

    typedef struct {
        enum class Type : uint8_t { Number, String, Boolean } Type;
        union {
            double Number;
            char const * String;
            bool Boolean;
        };
    } ScratchData;
   
    typedef struct {
        std::errc ec;
        ScratchData data;
    } Snum2DataResult;

    static Snum2DataResult __hot StringNum2ScratchData(std::string_view String) {
        if (unlikely(String.empty() == true)) {
            return { std::errc::invalid_argument, ScratchData() };
        }
        /* remove whitespace */
        while(String[0] == ' ') String.remove_prefix(1);
        while(String[String.length() - 1] == ' ') String.remove_suffix(1);

        if (String == "Infinity" || String == "+Infinity") {
            ScratchData Data = { .Type = ScratchData::Type::Number, .Number = std::numeric_limits<double>::infinity()};
            return { std::errc(), Data };
        } else if (String == "-Infinity") {
            ScratchData Data = { .Type = ScratchData::Type::Number, .Number = -std::numeric_limits<double>::infinity()};
            return { std::errc(), Data };
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
                ScratchData Data;
                Data.Type = ScratchData::Type::Number;
                if (Result.ec == std::errc::invalid_argument) {
                    /* bad num */
                    return { std::errc::invalid_argument, ScratchData() };
                } else if (Result.ec == std::errc::result_out_of_range) {
                    /* other possible result could be out of range (infinity for scratch) */
                    Data.Number = std::numeric_limits<double>::infinity();
                    return { std::errc(), Data };
                }
                Data.Number = double(NonDecConversion);
                return { std::errc(), Data };
            }
        }
SkipChecks:
        if (IS_INVALID_BASE10(String[String.length() - 1]) == true) {
            return { std::errc::invalid_argument, ScratchData() };
        }
        bool PastEuler = false;
        for(size_t i = 0; i < String.length(); i++) {
            const char c = String[i];
            if (IS_VALID_BASE10(c) == false) {
                return { std::errc::invalid_argument, ScratchData() };
            }
            if (PastEuler) {
                if (c == '.') {
                    return { std::errc::invalid_argument, ScratchData() };
                }
            }
            if (c == 'e' || c == 'E') {
                PastEuler = true;
            }
        }
        double ConvertedBase10;
        std::from_chars_result Result = std::from_chars(String.begin(), String.end(), ConvertedBase10);
        if (Result.ec == std::errc::invalid_argument) {
            return { std::errc::invalid_argument, ScratchData() };
        } else if (Result.ec == std::errc::result_out_of_range) {
            ScratchData Data;
            Data.Type = ScratchData::Type::Number;
            Data.Number = (String[0] == '-') ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
            return { std::errc(), Data };
        }
        /* could be nan or infinity */
        if (unlikely(std::isnan(ConvertedBase10) == true)) {
            ScratchData Data;
            Data.Type = ScratchData::Type::String;
            Data.String = "NaN";
            return { std::errc(), Data };
        }

        ScratchData Data;
        Data.Type = ScratchData::Type::Number;
        Data.Number = ConvertedBase10;
        return { std::errc(), Data };
    }


    inline bool __hot StringIsNumber(std::string_view String) {
        Snum2DataResult Result = StringNum2ScratchData(String);
        if (Result.ec == std::errc::invalid_argument) {
            return false;
        }
        return true;
    }

    static inline ScratchData __hot SanitizeData(ondemand::value VariableData) {
        ScratchData Data;
        switch(VariableData.type()) {
            case ondemand::json_type::string: {
                Data.Type = ScratchData::Type::String;
                /* not too reliable to detect strings. could be hex, octal, binary, or a number. */
                std::string SanitizedString(VariableData.get_string().value());
                if (StringIsNumber(SanitizedString) == true) {
                    Snum2DataResult Result = StringNum2ScratchData(SanitizedString);
                    if (Result.ec == std::errc::invalid_argument) {
                        /* normal string, not a number */
                        Data.String = SanitizedString.c_str();
                        return Data;
                    }
                    return Result.data;
                }
                /* could be hex or binary */
                Data.String = SanitizedString.c_str();
                return Data;
            }
            case ondemand::json_type::boolean: {
                bool SanitizedBool = VariableData.get_bool().value();
                Data.Type = ScratchData::Type::Boolean;
                Data.Boolean = SanitizedBool;
                return Data;
            }
            case ondemand::json_type::number: {
                /* various number types */
                Data.Type = ScratchData::Type::Number;
                switch(VariableData.get_number_type()) {
                    case ondemand::number_type::big_integer: {
                        Data.Number = double(0);
                        return Data;
                    }
                    case ondemand::number_type::floating_point_number: {
                        double SanitizedDouble = VariableData.get_double().value();
                        Data.Number = SanitizedDouble;
                        return Data;
                    }
                    default: {
                        double SanitizedNum = VariableData.get_number()->as_double();
                        Data.Number = SanitizedNum;
                        return Data;
                    }
                }
            }
            default: {
                Data.Type = ScratchData::Type::Number;
                Data.Number = 0;
                return Data;
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
                this->Public = IsPublic;
                VariableData.reset();
                ScratchData Sanitized = SanitizeData(VariableData.at(1).value());
                this->SetData(Sanitized);
                VariableData.reset();
            }
            inline void __hot SetData(ScratchData &NewData) {
                this->Data = NewData;
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
                DataJson = ListData.at(1).get_array();
                ListData.reset();
                DataJson.reset();
                this->TotalItems = DataJson.count_elements();
                if (this->TotalItems >= 200000) {
                    std::cout << "WARNING: List \"" << this->Name << "\" goes over 200,000 items; memory usage is bound to increase." << std::endl;
                }
                /* we dont load it. we lazily load things on-demand */
            }
            size_t TotalItems = 0;
            inline ScratchData __hot Get(size_t index) {
                if (index > this->TotalItems) {
                    return { .Type = ScratchData::Type::String, .String = "" };
                }
                ScratchData Data;
                if (Elements.size() < index) {
                    Data = SanitizeData(DataJson.at(index));
                    DataJson.reset();
                    Elements.emplace(Elements.begin() + index, Data);
                } else {
                    Data = Elements[index];
                }
                return Data;
            }
        private:
            ondemand::array DataJson;
            std::vector<ScratchData> Elements;
    };

    namespace Variables {
        void RegisterAll(void);
    };
};
