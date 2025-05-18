using System.Runtime.InteropServices;

[DllImport("__Internal")] static extern void console_ensure_init();
[DllImport("__Internal")] static extern void console_update();

bool IsSwitch = OperatingSystem.IsOSPlatform("libnx");

if (IsSwitch)
	console_ensure_init();

Console.WriteLine("Hello world from a C# application.");
Console.WriteLine($"Are we on switch? {IsSwitch}");

var rng = new Random();
var num = rng.Next(1,11);

Console.WriteLine();
Console.WriteLine("Can you guess the number I'm thinking?");
Console.WriteLine();

for (int i = 10; i > 1; i--)
{
	Console.WriteLine($"You have {i} seconds");

	if (IsSwitch) 
		console_update();
	
	Thread.Sleep(1000);
}

Console.WriteLine();
Console.WriteLine($"The number was {num}.");
Console.WriteLine("Thank you for playing.");

if (IsSwitch) 
	console_update();

Thread.Sleep(10000);