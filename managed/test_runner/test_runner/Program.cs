using System.Reflection;
using Microsoft.DotNet.XHarness.TestRunners.Common;
using Microsoft.DotNet.XHarness.TestRunners.Xunit;

// Based on SimpleAndroidTestRunner.cs from the runtime repo
public class LibnxTestRunner : AndroidApplicationEntryPoint, IDevice
{
	private static List<string> s_testLibs = new List<string>();
    private static string? s_MainTestName;

    public static async Task<int> Main(string[] args)
    {
        s_testLibs = Directory.GetFiles("sdmc:/mono/test", "*.Tests.dll", SearchOption.AllDirectories).ToList();
		Console.WriteLine($"Found {s_testLibs.Count} test libs.");
		
		if (s_testLibs.Count < 1)
        {
            Console.WriteLine($"Test libs were not found.");
            return -1;
        }

        int exitCode = 0;
        s_MainTestName = Path.GetFileNameWithoutExtension(s_testLibs[0]);
        string? verbose = Environment.GetEnvironmentVariable("XUNIT_VERBOSE")?.ToLower();
        bool enableMaxThreads = (Environment.GetEnvironmentVariable("XUNIT_SINGLE_THREADED") != "1");
        var simpleTestRunner = new LibnxTestRunner(verbose == "true" || verbose == "1", enableMaxThreads);
        
		simpleTestRunner.TestsCompleted += (e, result) => 
        {			
        	Console.WriteLine("----- TestsCompleted() called -----");
			Console.WriteLine($"Total: {result.TotalTests}, Passed: {result.PassedTests}, Failed: {result.FailedTests}, Skipped: {result.SkippedTests}");

            if (result.FailedTests > 0)
                exitCode = 1;
        };

        await simpleTestRunner.RunAsync();
        Console.WriteLine("----- Done -----");
        return exitCode;
    }

    public LibnxTestRunner(bool verbose, bool enableMaxThreads)
    {
        MinimumLogLevel = (verbose) ? MinimumLogLevel.Verbose : MinimumLogLevel.Info;
        _maxParallelThreads = (enableMaxThreads) ? Environment.ProcessorCount : 1;

        Console.WriteLine($"XUNIT: using {_maxParallelThreads} threads");
    }

    protected override IEnumerable<TestAssemblyInfo> GetTestAssemblies()
    {
        foreach (string file in s_testLibs)
        {
            yield return new TestAssemblyInfo(Assembly.LoadFrom(file), file);
        }
    }

    protected override void TerminateWithSuccess()
	{
		Console.WriteLine("TerminateWithSuccess() called");
	}

    private int? _maxParallelThreads;

    protected override int? MaxParallelThreads => _maxParallelThreads;

    protected override IDevice Device => this;

    protected override string? IgnoreFilesDirectory => null;

    protected override string IgnoredTraitsFilePath => "xunit-excludes.txt";

    public string BundleIdentifier => "net.dot." + s_MainTestName;

    public string? UniqueIdentifier => "libnx";
    public string? Name => "libnx";
    public string? Model => "libnx";
    public string? SystemName => "libnx";
    public string? SystemVersion => "libnx";
    public string? Locale { get; }

    public override TextWriter? Logger => null;

    public override string TestsResultsFinalPath
    {
        get
        {
            string? testResultsDir = "sdmc:/mono/test_results";
            if (string.IsNullOrEmpty(testResultsDir))
                throw new ArgumentException("TEST_RESULTS_DIR should not be empty");

            return Path.Combine(testResultsDir, "testResults.xml");
        }
    }
}