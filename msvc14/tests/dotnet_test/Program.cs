using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using log4cplus;

namespace dotnet_test
{
    class Program
    {
        public static DateTime UnixTimeStampToDateTime(ulong unixTimeStamp, ulong microseconds)
        {
            // Unix timestamp is seconds past epoch
            System.DateTime dtDateTime = new DateTime(1970, 1, 1, 0, 0, 0, 0, System.DateTimeKind.Utc);
            dtDateTime = dtDateTime
                .AddSeconds(unixTimeStamp)
                .AddMilliseconds((double)microseconds/1000)
                .ToLocalTime();
            return dtDateTime;
        }

        public static void callback(IntPtr cookie,
            string message, string loggerName, int log_level, string thread,
            string thread2, ulong timestamp_secs, uint timestamp_usecs,
            string file, string function, int line)
        {            
            Console.WriteLine("{2} [{0}] {1}", loggerName, message,
                UnixTimeStampToDateTime(timestamp_secs, timestamp_usecs));
        }

        static void Main(string[] args)
        {
            IntPtr initializer = CAPI.log4cplus_initialize();
            try
            {
                //CAPI.log4cplus_basic_configure();
                CAPI.log4cplus_add_callback_appender(null, callback, IntPtr.Zero);
                CAPI.log4cplus_logger_log_str("test", (int)LogLevel.INFO,
                    "logged this from C# console app through log4cplus C++ native library");
            }
            finally
            {
                CAPI.log4cplus_deinitialize(initializer);
            }
        }
    }
}
