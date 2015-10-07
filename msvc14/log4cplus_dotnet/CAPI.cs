using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

namespace log4cplus
{
    public enum LogLevel : int
    {
        OFF = 60000,
        FATAL = 50000,
        ERROR = 40000,
        WARN = 30000,
        INFO = 20000,
        DEBUG = 10000,
        TRACE = 0,
        ALL = TRACE,
        NOT_SET = -1
    };

    public sealed class CAPI
    {
#if LOG4CPLUS_RELEASE
        [DllImport("log4cplusU.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#elif LOG4CPLUS_DEBUG
        [DllImport("log4cplusUD.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#else
#error "neither LOG4CPLUS_RELEASE nor LOG4CPLUS_DEBUG is defined"
#endif
        public static extern IntPtr log4cplus_initialize();

#if LOG4CPLUS_RELEASE
        [DllImport("log4cplusU.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#elif LOG4CPLUS_DEBUG
        [DllImport("log4cplusUD.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#endif
        public static extern int log4cplus_deinitialize(IntPtr initializer);

#if LOG4CPLUS_RELEASE
        [DllImport("log4cplusU.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#elif LOG4CPLUS_DEBUG
        [DllImport("log4cplusUD.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#endif
        public static extern int log4cplus_file_configure(string pathname);

#if LOG4CPLUS_RELEASE
        [DllImport("log4cplusU.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#elif LOG4CPLUS_DEBUG
        [DllImport("log4cplusUD.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#endif
        public static extern int log4cplus_str_configure(string config);

#if LOG4CPLUS_RELEASE
        [DllImport("log4cplusU.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#elif LOG4CPLUS_DEBUG
        [DllImport("log4cplusUD.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#endif
        public static extern int log4cplus_basic_configure();

#if LOG4CPLUS_RELEASE
        [DllImport("log4cplusU.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#elif LOG4CPLUS_DEBUG
        [DllImport("log4cplusUD.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#endif
        public static extern void log4cplus_shutdown();

        public delegate void log4cplus_log_event_callback(IntPtr cookie,
            [In()] [MarshalAs(UnmanagedType.LPWStr)] string message,
            [In()] [MarshalAs(UnmanagedType.LPWStr)] string loggerName,
            int log_level,
            [In()] [MarshalAs(UnmanagedType.LPWStr)] string thread,
            [In()] [MarshalAs(UnmanagedType.LPWStr)] string thread2,
            ulong timestamp_secs, uint timestamp_usecs,
            [In()] [MarshalAs(UnmanagedType.LPWStr)] string file,
            [In()] [MarshalAs(UnmanagedType.LPWStr)] string function,
            int line);

#if LOG4CPLUS_RELEASE
        [DllImport("log4cplusU.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#elif LOG4CPLUS_DEBUG
        [DllImport("log4cplusUD.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#endif
        public static extern int log4cplus_add_callback_appender(
            string logger_name, log4cplus_log_event_callback callback, IntPtr cookie);

#if LOG4CPLUS_RELEASE
        [DllImport("log4cplusU.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#elif LOG4CPLUS_DEBUG
        [DllImport("log4cplusUD.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
#endif
        public static extern int log4cplus_logger_log_str(string name,
            int loglevel, string msg);
    }
}
