using System.Text;
using ExplorerDemo;
using ImGuiNET;
using System.Runtime.InteropServices;
using static SDL2.SDL;

public static class Program
{
    const int WindowWidth = 1280;
    const int WindowHeight = 720;

    // Default cimgui build does not include the SDL2 renderer, these are our additional imports
    [DllImport("cimgui")]
    static extern bool ImGui_ImplSDL2_InitForSDLRenderer(nint window, nint renderer);

    [DllImport("cimgui")]
    static extern bool ImGui_ImplSDLRenderer2_Init(nint renderer);

    [DllImport("cimgui")]
    static extern bool ImGui_ImplSDL2_ProcessEvent(in SDL_Event evt);

    [DllImport("cimgui")]
    static extern void ImGui_ImplSDLRenderer2_NewFrame();

    [DllImport("cimgui")]
    static extern void ImGui_ImplSDL2_NewFrame();

    [DllImport("cimgui")]
    static extern void ImGui_ImplSDLRenderer2_RenderDrawData(ImDrawDataPtr ptr);

    [DllImport("cimgui")]
    static extern void ImGui_ImplSDLRenderer2_Shutdown();

    [DllImport("cimgui")]
    static extern void ImGui_ImplSDL2_Shutdown();

    public static bool DebugEvents = false;

    private static unsafe string DebugSdlEvent(in SDL_Event evt)
    {
        var size = Marshal.SizeOf<SDL_Event>();
        StringBuilder sb = new();
        fixed (byte* data = evt.padding)
        {
            sb.Append($"SDL_Event: {evt.type} ");
            for (int i = 0; i < size; i++)
                sb.Append($"{data[i]:X2} ");
        }
        return sb.ToString();
    }

    public static void Main(string[] args)
    {
        Console.WriteLine("SDL_Init");
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK).AssertZero(SDL_GetError);

        Console.WriteLine("SDL_CreateWindow");
        var SdlWindow = SDL_CreateWindow("demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WindowWidth, WindowHeight, 0).AssertNotNull(SDL_GetError);

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

        Console.WriteLine("SDL_CreateRenderer");
        var SdlRenderer = SDL_CreateRenderer(SdlWindow, 0,
                SDL_RendererFlags.SDL_RENDERER_ACCELERATED |
                SDL_RendererFlags.SDL_RENDERER_PRESENTVSYNC)
                .AssertNotNull(SDL_GetError);

        SDL_GetRendererInfo(SdlRenderer, out var info);
        Console.WriteLine($"Initialized SDL with {Marshal.PtrToStringAnsi(info.name)} renderer");

        UpdateSize();

        Console.WriteLine("ImGui.CreateContext");
        var ctx = ImGui.CreateContext();

        if (File.Exists("OpenSans-Regular.ttf"))
            ImGui.GetIO().Fonts.AddFontFromFileTTF("OpenSans-Regular.ttf", 30);

        ImGui.GetIO().ConfigFlags |= ImGuiConfigFlags.NavEnableGamepad;

        unsafe
        {
            ImGui.GetIO().NativePtr->IniFilename = null;
        }

        Console.WriteLine("ImGui_ImplSDL2_InitForSDLRenderer");
        ImGui_ImplSDL2_InitForSDLRenderer(SdlWindow, SdlRenderer);

        Console.WriteLine("ImGui_ImplSDLRenderer2_Init");
        ImGui_ImplSDLRenderer2_Init(SdlRenderer);

        var demo = new FileExplorerDemo(Environment.CurrentDirectory);

        while (true)
        {
            while (SDL_PollEvent(out var evt) != 0)
            {
                if (DebugEvents)
                    Console.WriteLine(DebugSdlEvent(evt));

                if (evt.type == SDL_EventType.SDL_QUIT || evt.type == SDL_EventType.SDL_WINDOWEVENT &&
                    evt.window.windowEvent == SDL_WindowEventID.SDL_WINDOWEVENT_CLOSE ||
                    evt.type == SDL_EventType.SDL_KEYDOWN && evt.key.keysym.sym == SDL_Keycode.SDLK_ESCAPE)
                {
                    goto break_main_loop;
                }
                else if (evt.type == SDL_EventType.SDL_WINDOWEVENT && evt.window.windowEvent == SDL_WindowEventID.SDL_WINDOWEVENT_RESIZED)
                {
                    UpdateSize();
                }

                ImGui_ImplSDL2_ProcessEvent(in evt);
            }

            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui.NewFrame();

            demo.Render();

            ImGui.Render();
            SDL_SetRenderDrawColor(SdlRenderer, 0x0, 0x0, 0x0, 0xFF);
            SDL_RenderClear(SdlRenderer);
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui.GetDrawData());
            SDL_RenderPresent(SdlRenderer);
        }
    break_main_loop:

        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui.DestroyContext(ctx);

        SDL_DestroyRenderer(SdlRenderer);
        SDL_DestroyWindow(SdlWindow);
        SDL_Quit();

        void UpdateSize()
        {
            SDL_GetWindowSize(SdlWindow, out int w, out int h);
            if (OperatingSystem.IsMacOS())
            {
                SDL_GetRendererOutputSize(SdlRenderer, out int pixelWidth, out int pixelHeight);
                float scaleX = pixelWidth / (float)w;
                float scaleY = pixelHeight / (float)h;
                SDL_RenderSetScale(SdlRenderer, scaleX, scaleY);
            }
        }
    }
}