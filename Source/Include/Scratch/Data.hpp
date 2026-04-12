#pragma once

#include <Tachyon/Debug.hpp>
#include <Compiler.hpp>
#include <charconv>
#include <cmath>
#include <limits>
#include <string_view>
#include <string>
#include <variant>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wredundant-decls"
#include <Lib/SIMDJson.h>
#pragma GCC diagnostic pop

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

        /* value -> string conversion */
        std::string __hot AsString(void) {
            switch(this->Type) {
                case ScratchData::Type::String: {
                    return this->String;
                }
                case ScratchData::Type::Boolean: {
                    return this->Boolean ? "true" : "false";
                }
                case ScratchData::Type::Number: {
                    if (std::isinf(this->Number)) {
                        return this->Number < 0 ? "-Infinity" : "Infinity";
                    } else if (std::isnan(this->Number)) {
                        return "NaN";
                    }
                    char Buffer[64];
                    std::to_chars_result Result = std::to_chars(Buffer, Buffer + sizeof(Buffer), this->Number, std::chars_format::fixed);
                    return std::string(Buffer, Result.ptr);
                }
            }
            __unreachable;
        }

        /* value -> double conversion */
        double __hot AsDouble(void) {
            switch(this->Type) {
                case ScratchData::Type::String: {
                    return double(0);
                }
                case ScratchData::Type::Boolean: {
                    return this->Boolean ? 1 : 0;
                }
                case ScratchData::Type::Number: {
                    return this->Number;
                }
            }
            __unreachable;
        }

        friend std::ostream & operator << (std::ostream & Stream, ScratchData & Self) {
            Stream << Self.AsString();
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
        const ScratchData data;
        const std::errc ec;
    } Snum2DataResult;

    inline Snum2DataResult __hot StringNum2ScratchData(std::string_view String) {
        if (unlikely(String.empty() == true)) {
            return { ScratchData(), std::errc::invalid_argument };
        }
        /* remove whitespace in front */
        while(String[0] == ' ' && String.size() > 1) String.remove_prefix(1);

        if (unlikely(String.empty() == true)) {
            return { ScratchData(), std::errc::invalid_argument };
        }
        /* remove whitespace in back */
        while(String[String.length() - 1] == ' ' && String.size() > 1) String.remove_suffix(1);

        if (String == "Infinity" || String == "+Infinity") {
            return { ScratchData(std::numeric_limits<double>::infinity()), std::errc()};
        } else if (String == "-Infinity") {
            return { ScratchData(-std::numeric_limits<double>::infinity()), std::errc() };
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
                    return { ScratchData(), std::errc::invalid_argument };
                } else if (Result.ec == std::errc::result_out_of_range) {
                    /* other possible result could be out of range (infinity for scratch) */
                    return { ScratchData(std::numeric_limits<double>::infinity()), std::errc() };
                }
                return { ScratchData(double(NonDecConversion)), std::errc() };
            }
        }
SkipChecks:
        if (IS_INVALID_BASE10(String[String.length() - 1]) == true) {
            return { ScratchData(), std::errc::invalid_argument };
        }
        bool PastEuler = false;
        for(size_t i = 0; i < String.length(); i++) {
            const char c = String[i];
            if (IS_VALID_BASE10(c) == false) {
                return { ScratchData(), std::errc::invalid_argument };
            }
            if (PastEuler) {
                if (c == '.') {
                    return { ScratchData(), std::errc::invalid_argument };
                }
            }
            if (c == 'e' || c == 'E') {
                PastEuler = true;
            }
        }
        double ConvertedBase10;
        std::from_chars_result Result = std::from_chars(String.begin(), String.end(), ConvertedBase10);
        if (Result.ec == std::errc::invalid_argument) {
            return { ScratchData(), std::errc::invalid_argument };
        } else if (Result.ec == std::errc::result_out_of_range) {
            return { ScratchData((String[0] == '-') ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity()), std::errc() };
        }
        /* could be nan or infinity */
        if (unlikely(std::isnan(ConvertedBase10) == true)) {
            return { std::numeric_limits<double>::quiet_NaN(), std::errc() };
        }
        return { ScratchData(ConvertedBase10), std::errc() };
    }

    inline bool __hot StringIsNumber(std::string_view String) {
        Snum2DataResult Result = StringNum2ScratchData(String);
        if (Result.ec == std::errc::invalid_argument) {
            return false;
        }
        return true;
    }

    static ScratchData __hot SanitizeData(ondemand::value VariableData) {
        simdjson::ondemand::json_type ValueType;
        TachyonAssert(VariableData.type().get(ValueType) == error_code::SUCCESS);
        switch(ValueType) {
            case ondemand::json_type::string: {
                /* not too reliable to detect strings. could be hex, octal, binary, or a number. */
                std::string SanitizedString;
                TachyonAssert(VariableData.get_string().get(SanitizedString) == error_code::SUCCESS);
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
                bool SanitizedBool;
                TachyonAssert(VariableData.get_bool().get(SanitizedBool) == error_code::SUCCESS);
                return ScratchData(SanitizedBool);
            }
            case ondemand::json_type::number: {
                /* various number types */
                simdjson::ondemand::number_type NumberType;
                TachyonAssert(VariableData.get_number_type().get(NumberType) == error_code::SUCCESS);
                switch(NumberType) {
                    case ondemand::number_type::big_integer: {
                        return ScratchData(std::numeric_limits<double>::infinity());
                    }
                    default: {
                        double SanitizedNum;
                        TachyonAssert(VariableData.get_double().get(SanitizedNum) == error_code::SUCCESS);
                        return ScratchData(SanitizedNum);
                    }
                }
            }
            default: {
                return double(0);
            }
        }
    }

    class ScratchVariable_Base {
        public:
            constexpr std::string & GetName(void) {
                return this->Name;
            }
            constexpr bool IsPublic(void) {
                return this->Public;
            }
        protected:
            std::string Name;
            bool Public;
    };

    class ScratchVariable : public ScratchVariable_Base {
        public:
            ScratchVariable(ondemand::array VariableData, const bool IsPublic) {
                /* VariableData[0] = list name, VariableData[1] = actual data */
                simdjson::simdjson_result Result = VariableData.at(0);
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_string().get(this->Name) == error_code::SUCCESS);

                VariableData.reset();
                Result = VariableData.at(1);

                TachyonAssert(Result.error() == error_code::SUCCESS);
                ondemand::value RawValue;
                TachyonAssert(Result.get(RawValue) == error_code::SUCCESS);
                this->Data = SanitizeData(RawValue);

                VariableData.reset();
                this->Public = IsPublic;
            }

            inline void __hot SetData(ScratchData && NewData) {
                this->Data = NewData;
            }
            constexpr ScratchData & __hot GetData(void) {
                return Data;
            }
        private:
            ScratchData Data;
    };

    class ScratchList : public ScratchVariable_Base {
        public:
            ScratchList (ondemand::array ListData, const bool IsPublic) {
                /* same applies for ScratchList as it does for ScratchVariable; 
                 * ListData[0] = list name, ListData[1] = [actual data] */
                
                simdjson::simdjson_result Result = ListData.at(0);
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_string().get(this->Name) == error_code::SUCCESS);
                /* prepare for lazy loading */
                ListData.reset();
                Result = ListData.at(1);

                ondemand::array ListArray;
                TachyonAssert(Result.error() == error_code::SUCCESS);
                TachyonAssert(Result.get_array().get(ListArray) == error_code::SUCCESS);
                TachyonAssert(ListArray.count_elements().get(this->TotalItems) == error_code::SUCCESS);

                std::string RawJsonString;
                TachyonAssert(ListArray.raw_json().get(RawJsonString) == error_code::SUCCESS);
                this->ListJson = padded_string(RawJsonString);

                ListData.reset();

                if (this->TotalItems > 200000) {
                    DebugWarn("List \"%s\" goes over 200,000 items; memory usage is bound to increase. To reduce memory usage, consider using pseudo-blocks to create a less memory-expensive buffer (if the list consists of number values under 256).\n", this->Name.c_str());
                }
                this->LazyLoad = true;
                this->Public = IsPublic;
                this->Size = this->TotalItems;
            }

            void __hot SwitchToBuffer(void) {
                if (unlikely(std::holds_alternative<uint8_t *>(this->Elements) == true)) {
                    /* you're already a buffer pal */
                    return;
                }

                if (unlikely(this->TotalItems == 0)) {
                    DebugError("Please use this function on a populated list.\n");
                    return;
                }

                uint8_t * NewBuffer = new (std::nothrow) uint8_t[this->TotalItems];

                if (unlikely(NewBuffer == nullptr)) {
                    DebugError("Failed to allocate UINT8 buffer for \"%s\"\n", this->Name.c_str());
                    return;
                }

                std::vector<ScratchData> & Cache = std::get<std::vector<ScratchData>>(this->Elements);

                ondemand::parser JsonParser;
                simdjson::simdjson_result DataDoc = JsonParser.iterate(this->ListJson);
                TachyonAssert(DataDoc.error() == error_code::SUCCESS);
                ondemand::array DataArray;
                TachyonAssert(DataDoc.get_array().get(DataArray) == error_code::SUCCESS);

                size_t i = 0;
                for(auto Value : DataArray) {
                    ondemand::value RawData;
                    TachyonAssert(Value.get(RawData) == error_code::SUCCESS);
                
                    ScratchData Item = SanitizeData(RawData);

                    if (unlikely(Item.Type != ScratchData::Type::Number || Item.Number > 255)) {
                        DebugError("Failed to load buffer \"%s\": The list NEEDS to only have numbers, and they can't be greater than 255.\n", this->Name.c_str());
                        delete[] NewBuffer;
                        return;
                    }

                    uint8_t Byte(Item.Number);
                    NewBuffer[i] = Byte;
                    i++;
                }
                /* bye bye vector */
                Cache.clear();
                this->Elements = NewBuffer;
                this->LazyLoad = false;
                DebugInfo("Buffer \"%s\" is ready for use.\n", this->Name.c_str());
            }

            void __hot ClearElements(void) {
                if (std::holds_alternative<std::vector<ScratchData>>(this->Elements)) {
                    std::vector<ScratchData> & ElementVector = std::get<std::vector<ScratchData>>(this->Elements);
                    ElementVector.clear();
                    this->TotalItems = 0;
                    this->LazyLoad = false;
                    return;
                }
                uint8_t * Buffer = std::get<uint8_t *>(this->Elements);
                memset(Buffer, 0, this->Size);
                this->TotalItems = 0;
            }

            ScratchData __hot Get(const size_t Index) {
                if (unlikely(Index >= this->TotalItems)) {
                    return std::string();
                }
                if (std::holds_alternative<std::vector<ScratchData>>(this->Elements)) {
                    std::vector<ScratchData> & ListVector = std::get<std::vector<ScratchData>>(this->Elements);
                    if (this->LazyLoad == true) {
                        if (likely(Index < ListVector.size())) {
                            /* cache HIT */
                            //DebugInfo("CACHE HIT\n");
                            return ListVector.at(Index);
                        }
                        /* cache miss */
                        ondemand::parser JsonParser;
                        auto DataArray = JsonParser.iterate(this->ListJson);
                        TachyonAssert(DataArray.error() == error_code::SUCCESS);

                        ondemand::value RawData;
                        TachyonAssert(DataArray.at(Index).get(RawData) == error_code::SUCCESS);

                        ScratchData Data = SanitizeData(RawData);
                        ListVector.resize(Index + 1);
                        ListVector.insert(ListVector.begin() + Index, Data);
                        //DebugInfo("CACHE MISS\n");
                        return Data;
                    }
                    /* lazy load off = all items loaded in the vector */
                    return ListVector.at(Index);
                } else {
                    uint8_t * Buffer = std::get<uint8_t *>(this->Elements);
                    return double(Buffer[Index]);
                }
                __unreachable;
            }

            void __hot Set(const ScratchData && Data, const size_t Index) {
                if (unlikely(Index >= this->TotalItems)) {
                    return;
                }
                if (std::holds_alternative<std::vector<ScratchData>>(this->Elements)) {
                    std::vector<ScratchData> & ListVector = std::get<std::vector<ScratchData>>(this->Elements);
                    if (this->LazyLoad == true) {
                        if (likely(Index < ListVector.size())) {
                            ListVector[Index] = Data;
                            return;
                        }
                        ListVector.resize(Index);
                        ListVector.insert(ListVector.begin() + Index, Data);
                        return;
                    }
                    ListVector[Index] = Data;
                    return;
                }
                uint8_t * Buffer = std::get<uint8_t *>(this->Elements);
                if (unlikely(Data.Type != ScratchData::Type::Number || Data.Number > 255)) {
                    DebugWarn("If you're going to write data into the buffer, please write a number value under 256, otherwise nothing will be written to the buffer.\n");
                }
                Buffer[Index] = uint8_t(Data.Number);
            }

            void __hot Append(const ScratchData && Data) {
                if (std::holds_alternative<std::vector<ScratchData>>(this->Elements)) {
                    /* lazy loading is no longer useful */
                    std::vector<ScratchData> & ListVector = std::get<std::vector<ScratchData>>(this->Elements);

                    if (this->LazyLoad == true) {
                        this->TotalItems++;

                        ListVector.clear();
                        ListVector.resize(this->TotalItems);

                        ondemand::parser JsonParser;
                        simdjson::simdjson_result DataDoc = JsonParser.iterate(this->ListJson);
                        TachyonAssert(DataDoc.error() == error_code::SUCCESS);
                        ondemand::array DataArray;
                        TachyonAssert(DataDoc.get_array().get(DataArray) == error_code::SUCCESS);

                        size_t i = 0;
                        for(auto Value : DataArray) {
                            ondemand::value RawData;
                            TachyonAssert(Value.get(RawData) == error_code::SUCCESS);
                        
                            ScratchData Item = SanitizeData(RawData);

                            ListVector.insert(ListVector.begin() + i, Item);
                            i++;
                        }
                        ListVector.insert(ListVector.begin() + this->TotalItems - 1, Data);
                        this->LazyLoad = false;
                        return;
                    }
                    this->TotalItems++;
                    if (this->TotalItems > this->Size) {
                        this->Size++;
                    }
                    ListVector.resize(this->TotalItems);
                    ListVector.insert(ListVector.begin() + this->TotalItems - 1, Data);
                    return;
                }
                DebugWarn("Cannot append items to buffer.\n");
            }
            size_t TotalItems = 0;
            size_t Size = 0;
        private:
            std::variant<std::vector<ScratchData>, uint8_t *> Elements;
            padded_string ListJson;
            bool LazyLoad;
    };

    namespace Data {
        void RegisterAll(void);
    };
};
