#pragma once

namespace Tachyon {
    namespace Time {
        double GetSeconds(void);
        double GetMinute(void);
        double GetHour(void);
        double GetDayOfWeek(void);
        double GetDayOfMonth(void);
        double GetMonth(void);
        double GetYear(void);
        
        double GetDaysSince2000(void);
    };
};