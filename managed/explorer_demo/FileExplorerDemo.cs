using ImGuiNET;
using System.Numerics;
using static SDL2.SDL;

namespace ExplorerDemo
{
    internal class FileExplorerDemo
    {
        bool DemoWindow = false;
        bool HelpWindow = false;

        string Cwd = null!;
        DirectoryInfo[] Directories = [];
        FileInfo[] Files = [];

        string? Error = null;

        public FileExplorerDemo(string initialPath)
        {
            ChangeDirectory(initialPath);
        }

        public void ChangeDirectory(string path)
        {
            try
            {
                Cwd = path;
                var info = new DirectoryInfo(path);
                Directories = info.GetDirectories();
                Files = info.GetFiles();
                Error = null;
            }
            catch (Exception e)
            {
                Error = e.Message;
                Directories = [];
                Files = [];
                Cwd = "/";
                return;
            }
        }

        public string HumanSize(long bytes)
        {
            string[] sizes = { "B", "KB", "MB", "GB", "TB" };
            double len = bytes;
            int order = 0;
            while (len >= 1024 && order < sizes.Length - 1)
            {
                order++;
                len /= 1024;
            }
            return $"{len:0.##} {sizes[order]}";
        }

        public void Render()
        {
            FileExplorerWindow();

            if (DemoWindow)
                ImGui.ShowDemoWindow(ref DemoWindow);

            if (HelpWindow)
                AboutWindow();
        }

        void AboutWindow() 
        {
            var screenSize = ImGui.GetIO().DisplaySize;
            var windowSize = new Vector2(650, 300);

            ImGui.SetNextWindowSize(windowSize, ImGuiCond.FirstUseEver);
            ImGui.SetNextWindowPos((screenSize - windowSize) / 2, ImGuiCond.FirstUseEver);
            if (!ImGui.Begin("About", ref HelpWindow))
                return;

            ImGui.Text("SDL2 + Imgui C# demo for mono-nx");
            ImGui.Text("https://github.com/exelix11/mono-nx");
            ImGui.NewLine();
            ImGui.Text("Third party libraries:");
            ImGui.Text("https://github.com/ImGuiNET/ImGui.NET");
            ImGui.Text("https://github.com/flibitijibibo/SDL2-CS");
            ImGui.End();
        }

        void FileExplorerWindow()
        {
            ImGui.SetNextWindowSize(new Vector2(800, 600), ImGuiCond.FirstUseEver);
            ImGui.SetNextWindowPos(new Vector2(50, 50), ImGuiCond.FirstUseEver);

            if (!ImGui.Begin("File Explorer", ImGuiWindowFlags.MenuBar))
                return;

            if (ImGui.BeginMenuBar())
            {
                if (ImGui.BeginMenu("File"))
                {
                    if (ImGui.MenuItem("Show imgui demo window"))
                    {
                        DemoWindow = !DemoWindow;
                    }
                    if (ImGui.MenuItem("Debug events"))
                    {
                        Program.DebugEvents = !Program.DebugEvents;
                    }
                    if (ImGui.MenuItem("Exit"))
                    {
                        SDL_Event quitEvent = new SDL_Event { type = SDL_EventType.SDL_QUIT };
                        SDL_PushEvent(ref quitEvent);
                    }
                    ImGui.EndMenu();
                }

                if (ImGui.BeginMenu("Help"))
                {
                    if (ImGui.MenuItem("About"))
                        HelpWindow = !HelpWindow;
                    ImGui.EndMenu();
                }

                if (ImGui.BeginMenu($"FPS: {ImGui.GetIO().Framerate}"))
                {
                    ImGui.EndMenu();
                }

                ImGui.EndMenuBar();
            }

            ImGui.Text($"Current Directory: {Cwd}");

            if (Error != null)
            {
                ImGui.TextColored(new Vector4(1, 0, 0, 1), $"Error: {Error}");
                return;
            }

            if (ImGui.BeginChild("FileList", new Vector2(0, 0), true))
            {
                // Add the ".." entry to go up a directory  
                if (ImGui.Selectable(".."))
                    ChangeDirectory(Path.GetDirectoryName(Cwd) ?? Path.GetPathRoot(Cwd) ?? "/");

                var columnSize = ImGui.GetWindowSize().X * .8f;

                foreach (var dir in Directories)
                {
                    ImGui.Columns(2);
                    ImGui.SetColumnWidth(0, columnSize);
                    if (ImGui.Selectable($"{dir.Name}/"))
                    {
                        ChangeDirectory(dir.FullName);
                    }
                    ImGui.NextColumn();
                    ImGui.Text($"[DIR]");
                    ImGui.Columns(1);
                }

                foreach (var file in Files)
                {
                    ImGui.Columns(2);
                    ImGui.SetColumnWidth(0, columnSize);
                    ImGui.Selectable($"{file.Name}");
                    ImGui.NextColumn();
                    ImGui.Text($"{HumanSize(file.Length)}");
                    ImGui.Columns(1);
                }

                ImGui.EndChild();
            }

            ImGui.End();
        }
    }
}
