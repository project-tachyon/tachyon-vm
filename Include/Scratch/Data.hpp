#ifndef TACHYON_SCRATCH_DATA_HPP
#define TACHYON_SCRATCH_DATA_HPP

#include <Compiler.h>
#include <variant>
#include <string>
#include <Lib/SIMDJson.h>

using namespace simdjson;

typedef std::variant<double, std::string, bool> ScratchData;

class ScratchVariable_Base {
    public:
        std::string_view GetName(void) {
            return this->Name;
        }
        bool IsPublic(void) {
            return this->Public;
        }
    protected:
        std::string_view Name;
        bool Public;
};

class ScratchVariable : ScratchVariable_Base {
    public:
        ScratchVariable(ondemand::array VariableData, bool IsPublic) {
            /* VariableData[0] = list name, VariableData[1] = actual data */
            this->Name = VariableData.at(0).get_string();
            this->Public = IsPublic;
            VariableData.reset();
            ScratchData Sanitized = this->SanitizeData(VariableData.at(1).value());
            this->SetData(Sanitized);
            VariableData.reset();
        }
        inline void __hot SetData(ScratchData &NewData) {
            this->Data = NewData;
        }
    private:
        inline ScratchData __hot SanitizeData(ondemand::value VariableData) {
            switch(VariableData.type()) {
                case ondemand::json_type::string: {
                    std::string SanitizedString(VariableData.get_string()->data());
                    return ScratchData(SanitizedString);
                }
                case ondemand::json_type::boolean: {
                    bool SanitizedBool = VariableData.get_bool();
                    return SanitizedBool;
                }
                case ondemand::json_type::number: {
                    /* various number types */
                    switch(VariableData.get_number_type()) {
                        case ondemand::number_type::big_integer: {
                            std::cout << "caught a big integer in the wild!!" << std::endl;
                            return double(67.69);
                        }
                        case ondemand::number_type::floating_point_number: {
                            double SanitizedDouble = VariableData.get_double();
                            return SanitizedDouble;
                        }
                        default: {
                            double SanitizedNum = VariableData.get_number()->as_double();
                            return SanitizedNum;
                        }
                    }
                }
                default: {
                    return "unknown";
                }
            }
        }
        ScratchData Data;
};

class ScratchList : ScratchVariable_Base {
    public:
        ScratchList(ondemand::array ListData, bool IsPublic) {
            /* same applies for ScratchList as it does for ScratchVariable; 
             * ListData[0] = list name, ListData[1] = actual data */
            this->Name = ListData.at(0)->get_string();
            ListData.reset();
            this->TotalItems = ListData.at(1)->count_elements();
            this->Public = IsPublic;
            ListData.reset();
        }
        size_t TotalItems;
    private:
        std::vector<ScratchData> Elements;
};

#endif
