using System.Runtime.InteropServices;

if (!OperatingSystem.IsOSPlatform("libnx"))
{ 
	Console.WriteLine("This example is only for mono-nx");
	return;
}

[DllImport("__Internal")] static extern void console_ensure_init();
[DllImport("__Internal")] static extern void console_update();

console_ensure_init();

LibnxPad pad = new();
while (LibnxApplet.appletMainLoop())
{
	pad.Update();

	if (pad.ButtonsDown.HasFlag(HidNpadButton.Plus))
		break;

	// The console class is not implemented so Console.Clear() will crash
	// However you can manually use terminal escape codes to clear the screen
	Console.Write("\x1b[1;1H\x1b[2J");

	Console.Write($"Buttons: {pad.Buttons}");
	Console.Write($"Left stick: x={pad.State.sticks_0}");
	Console.Write($"Right stick: x={pad.State.sticks_1}");

	Console.WriteLine("");
	Console.WriteLine("Press start to exit");

	// This locks us to the refresh rate
	console_update();
}