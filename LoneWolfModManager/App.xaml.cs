using System.Configuration;
using System.Data;
using System.Globalization;
using System.IO;
using System.Windows;
using Serilog;

namespace LoneWolfModManager
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            const string logFile = "mod_manager.log";
            if (File.Exists(logFile))
            {
                File.Delete(logFile);
            }
            Log.Logger = new LoggerConfiguration()
                .WriteTo.File(logFile)
                .CreateLogger();

            Log.Information("Starting LoneWolf Mod Manager...");

            var culture = CultureInfo.CurrentCulture;

            Thread.CurrentThread.CurrentCulture = culture;
            Thread.CurrentThread.CurrentUICulture = culture;
        }

        protected override void OnExit(ExitEventArgs e)
        {
            Log.Information("Exiting LoneWolf Mod Manager...");
            Log.CloseAndFlush();
            base.OnExit(e);
        }
    }
}
