using System.Runtime.InteropServices;

public class LibnxApplet
{
	[DllImport("libnx")]
	public static extern bool appletMainLoop();
}