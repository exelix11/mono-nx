using System.Net;
using System.Net.Sockets;
using System.Diagnostics;
using System.Text;
using System.Runtime.InteropServices;

namespace AotExample
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
			// For reasons I did not bother to figure out, this is needed in AOT builds.
			AppContext.SetSwitch("System.Resources.UseSystemResourceKeys", true);

			if (IsSwitch)
				console_ensure_init();

			Console.WriteLine($"Hello world from a C# application.");
			Console.WriteLine($"Are we on switch? {IsSwitch}");

			if (IsSwitch)
				console_update();

			Thread.Sleep(6000);

			return 0;
		}
	}
}