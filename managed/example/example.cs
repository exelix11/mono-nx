using System.Net;
using System.Net.Sockets;
using System.Diagnostics;
using System.Text;
using System.Runtime.InteropServices;

namespace Example
{
	public class Program
	{
		// What's this? Libnx's console is emulated. We need to call init to create the framebuffers and update to draw to the the screen
		// This is obviously not going to work on other operating systems.
		// This is not done automatically because otherwise it would cause problems to other libraries like SDL
		[DllImport("__Internal")] static extern void console_ensure_init();
		[DllImport("__Internal")] static extern void console_update();

		static bool IsSwitch = OperatingSystem.IsOSPlatform("libnx");

		public static int Main(string[] args)
		{
			// This is a workaround for an issue in the interpreter builds. It should not affect this specific program but it's left here as a reference. See the writeup for more info.
			AppContext.SetSwitch("System.Resources.UseSystemResourceKeys", true);

			if (IsSwitch)
				console_ensure_init();

			Console.WriteLine($"Hello world from a C# application.");
			Console.WriteLine($"Are we on switch? {IsSwitch}");

			if (IsSwitch)
				console_update();

			try
			{
				RunPrintTest();
				ListTest();
				DelegateTest();
				TestThreads();
				ThrowTest();
				StackTraceTest();
				TestFilesystem();
				TestAsync().GetAwaiter().GetResult();

				// These will fail in an emulator:
				UdpSocketTest();
				TCPHttpTest();
				HttpTest().Wait();

				Console.WriteLine($"DONE !");
			}
			catch (Exception ex)
			{
				Console.WriteLine($"Caught exception: {ex}");
				return 1;
			}

			Console.WriteLine("Closing in 10 seconds");

			if (IsSwitch)
				console_update();

			Thread.Sleep(10 * 1000);

			return 0;
		}

		public static int Add(int a, int b)
		{
			return a + b;
		}

		public static void ListTest()
		{
			var list = new List<int>();
			for (int i = 0; i < 10; i++)
				list.Add(i);

			foreach (var item in list)
				Console.WriteLine(item);
		}

		public static void DelegateTest()
		{
			var listOfFuncs = new List<Func<int, int>>();
			for (int i = 0; i < 10; i++)
			{
				int j = i;
				listOfFuncs.Add(x => x + j);
			}
			foreach (var func in listOfFuncs)
			{
				Console.WriteLine(func(10));
			}
		}

		public static void RunPrintTest()
		{
			Console.WriteLine("Hello, World!");
		}

		public static void StackTraceTest()
		{
			var a = new System.Diagnostics.StackTrace();
			Console.WriteLine("StackTrace: " + a.ToString());
		}

		public static void TestThreads()
		{
			var rng = new Random();
			var threads = new List<Thread>();
			int count = 0;

			for (int i = 0; i < 10; i++)
			{
				threads.Add(new Thread(() =>
				{
					var delay = 0;
					lock (rng)
						delay = rng.Next(1000);

					Thread.Sleep(delay);
					Interlocked.Increment(ref count);
				}));

				threads[i].Start();
			}

			foreach (var thread in threads)
				thread.Join();

			Console.WriteLine($"Count: {count}");
		}

		public static void ThrowTest()
		{
			try
			{
				var action = () => { throw new Exception("Test exception"); };
				action();
			}
			catch (Exception ex)
			{
				Console.WriteLine($"Caught exception: {ex}");
			}
		}

		public static async Task TestAsync()
		{
			try
			{
				Console.WriteLine("Async test started.");
				await Task.Delay(1000);
				Console.WriteLine("Async test completed.");
			}
			catch (Exception ex)
			{
				Console.WriteLine($"Exception: {ex}");
			}
		}

		public static void TestFilesystem()
		{
			Console.WriteLine("I'm in: " + Directory.GetCurrentDirectory());

			var filesInRoot = Directory.GetFiles("/");
			foreach (var file in filesInRoot)
			{
				Console.WriteLine(file);
			}

			if (!Directory.Exists("/mono_test_tmp"))
			{
				Directory.CreateDirectory("/mono_test_tmp");
				Console.WriteLine("Created /mono_test_tmp directory.");
			}

			var rng = new Random();
			var number = rng.Next(1000).ToString();
			var filePath = $"/mono_test_tmp/testfile.txt";

			File.WriteAllText(filePath, number);
			Console.WriteLine($"Wrote {number} to {filePath}");
			var readNumber = File.ReadAllText(filePath);
			Console.WriteLine($"Read {readNumber} from {filePath}");
			File.Delete(filePath);
			Console.WriteLine($"Deleted {filePath}");
			Directory.Delete("/mono_test_tmp");
			Console.WriteLine("Deleted /mono_test_tmp directory.");

			if (File.Exists("config.ini"))
			{
				Console.WriteLine("config.ini exists at: " + Path.GetFullPath("config.ini"));
				Console.WriteLine(File.ReadAllText("config.ini"));
			}
			else
			{
				Console.WriteLine("config.ini was not found");
			}
		}

		public static void UdpSocketTest()
		{
			using var s = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
			s.SendTo(Encoding.ASCII.GetBytes("hello UDP sockets"), new IPEndPoint(IPAddress.Parse("192.168.2.5"), 9999));
		}

		public static void TCPHttpTest()
		{
			//using var client = new TcpClient("google.com", 80);
			//using var stream = client.GetStream();
			using var socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
			socket.Connect("google.com", 80);

			using var stream = new NetworkStream(socket);
			using var writer = new StreamWriter(stream) { AutoFlush = true };
			using var reader = new StreamReader(stream);
			writer.WriteLine("GET / HTTP/1.1");
			writer.WriteLine("Host: google.com");
			writer.WriteLine("Connection: close");
			writer.WriteLine();
			writer.WriteLine();

			string? line;
			while ((line = reader.ReadLine()) != null)
				Console.WriteLine(line);
				
			Console.WriteLine("TCP test completed.");
		}

		public static async Task HttpTest()
		{
			HttpClientHandler handler = new HttpClientHandler();
			// Goole.com will redirect to https, but we don't support SSL
			handler.AllowAutoRedirect = false;

			using var client = new HttpClient(handler);
			var response = await client.GetAsync("http://google.com");
			var text = await response.Content.ReadAsStringAsync();

			Console.WriteLine($"Status Code: {response.StatusCode}");
			Console.WriteLine($"Response: {text}");
		}
	}
}