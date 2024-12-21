using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.IO.Pipes;
using System.Text;
using System.Text.Json;
using System.Windows;
using System.Windows.Data;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Serilog;

namespace LoneWolfModManager;

public class ModRegistry
{
    public string Directory { get; set; } = "";
    public ModData? Data { get; set; }
}

public partial class MainWindowView : ObservableObject
{
    private const string ApplicationIcon = "pack://application:,,,/hw3.ico";
    public BitmapImage ApplicationIconImage
    {
        get
        {
            var bitmap = new BitmapImage();
            bitmap.BeginInit();
            bitmap.UriSource = new Uri(ApplicationIcon, UriKind.RelativeOrAbsolute);
            bitmap.CacheOption = BitmapCacheOption.OnLoad;
            bitmap.EndInit();
            bitmap.Freeze();
            return bitmap;
        }
    }

    private const int ModManagerVersion = 1;

    public ObservableCollection<ModRegistry> NonActiveMods { get; } = [];
    public ObservableCollection<ModRegistry> ActiveMods { get; } = [];

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(LastSelectedMod))]
    [NotifyCanExecuteChangedFor(nameof(ActivateModCommand))]
    public partial ModRegistry? SelectedNonActiveMod { get; set; }
    partial void OnSelectedNonActiveModChanged(ModRegistry? value)
    {
        if (value is not null)
        {
            SelectedActiveMod = null;
            LastSelectedMod = value;
        }
    }

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(LastSelectedMod))]
    [NotifyCanExecuteChangedFor(nameof(DeActivateModCommand))]
    public partial ModRegistry? SelectedActiveMod { get; set; }
    partial void OnSelectedActiveModChanged(ModRegistry? value)
    {
        if (value is not null)
        {
            SelectedNonActiveMod = null;
            LastSelectedMod = value;
        }
    }

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(LastSelectedModPreviewImage))]
    public partial ModRegistry? LastSelectedMod { get; set; }

    private const string FallBackPreviewImage = "pack://application:,,,/PreviewImagePlaceholder.png";

    public BitmapImage LastSelectedModPreviewImage
    {
        get
        {
            var path = LastSelectedMod?.Data?.PreviewImage ?? FallBackPreviewImage;
            var bitmap = new BitmapImage();
            bitmap.BeginInit();
            bitmap.UriSource = new Uri(path, UriKind.RelativeOrAbsolute);
            bitmap.CacheOption = BitmapCacheOption.OnLoad;
            bitmap.EndInit();
            bitmap.Freeze();
            return bitmap;
        }
    }

    [RelayCommand(CanExecute = nameof(CanActivateMod))]
    public void ActivateMod()
    {
        var selected = SelectedNonActiveMod!;
        NonActiveMods.Remove(selected);
        ActiveMods.Add(selected);
    }

    public bool CanActivateMod() => SelectedNonActiveMod is not null;

    [RelayCommand(CanExecute = nameof(CanDeActivateMod))]
    public void DeActivateMod()
    {
        var selected = SelectedActiveMod!;
        ActiveMods.Remove(selected);
        NonActiveMods.Add(selected);
    }

    public bool CanDeActivateMod() => SelectedActiveMod is not null;

    private const string ManagedModsPath = "ManagedMods";
    private const string PipeName = "LoneWolfHW3ModManager";

    public MainWindowView()
    {
        if (!Directory.Exists(ManagedModsPath))
        {
            Directory.CreateDirectory(ManagedModsPath);
        }
        var modPaths = Directory.GetDirectories(ManagedModsPath);
        var options = new JsonSerializerOptions
        {
            PropertyNamingPolicy = JsonNamingPolicy.SnakeCaseLower,
            AllowTrailingCommas = true
        };
        foreach (var modPath in modPaths)
        {
            var modJsonPath = Path.Combine(modPath, "mod.json");
            if (!File.Exists(modJsonPath))
            {
                Log.Warning($"Mod info file not found: {modJsonPath}");
                continue;
            }
            try
            {
                var jsonContent = File.ReadAllText(modJsonPath);
                var modData = JsonSerializer.Deserialize<ModData>(jsonContent, options);
                if (modData is null)
                {
                    throw new JsonException("Failed to deserialize mod data");
                }
                if (modData.ModManagerVersion > ModManagerVersion)
                {
                    Log.Warning($"Mod requires newer version of mod manager: {modData.ModName}");
                }
                if (string.IsNullOrWhiteSpace(modData.PreviewImage))
                {
                    Log.Warning($"Mod has no preview image: {modData.ModName}");
                    modData.PreviewImage = null;
                }
                else
                {
                    var previewImagePath = Path.Combine(modPath, modData.PreviewImage);
                    if (!File.Exists(previewImagePath))
                    {
                        Log.Warning($"Preview image not found: {previewImagePath}");
                        modData.PreviewImage = null;
                    }
                    else
                    {
                        modData.PreviewImage = previewImagePath;
                    }
                }

                var modDirectory = Path.GetFileName(modPath);
                NonActiveMods.Add(new ModRegistry
                {
                    Data = modData,
                    Directory = modDirectory
                });
            }
            catch (JsonException ex)
            {
                Log.Error($"Error reading mod info {modJsonPath}: {ex.Message}");
            }
        }
    }

    private enum MessageType
    {
        ClientHello = 0,
        ServerHello = 1,
        RequestMappingList = 2,
        SetMappingList = 3,
    }

    private interface IMessage
    {
        public MessageType Type { get; }
    }

    private class BaseMessage : IMessage
    {
        public MessageType Type { get; init; }
    }

    private class ClientHelloMessage : IMessage
    {
        public MessageType Type => MessageType.ClientHello;
        public int ClientVersion { get; init; }
    }

    private class ServerHelloMessage : IMessage
    {
        public MessageType Type => MessageType.ServerHello;
        public int ServerVersion => ModManagerVersion;
    }

    private class RequestMappingListMessage : IMessage
    {
        public MessageType Type => MessageType.RequestMappingList;
    }

    private class Mapping
    {
        public string VirtualPath { get; init; } = "";
        public string RealPath { get; init; } = "";
    }

    private class SetMappingListMessage : IMessage
    {
        public MessageType Type => MessageType.SetMappingList;
        public List<Mapping> Mappings { get; init; } = [];
    }

    public async Task StartServer()
    {
        while (true)
        {
            var pipeServer = new NamedPipeServerStream(
                PipeName, PipeDirection.InOut, NamedPipeServerStream.MaxAllowedServerInstances,
                PipeTransmissionMode.Byte, PipeOptions.Asynchronous);
            Log.Information("Waiting client connection...");
            await pipeServer.WaitForConnectionAsync();
            Log.Information("Client connected.");
            _ = Task.Run(() => HandleClient(pipeServer));
        }
    }

    private void HandleClient(NamedPipeServerStream pipeServer)
    {
        try
        {
            using var server = pipeServer;
            using var reader = new BinaryReader(pipeServer);
            using var writer = new BinaryWriter(pipeServer);

            var options = new JsonSerializerOptions
            {
                PropertyNamingPolicy = JsonNamingPolicy.SnakeCaseLower
            };

            while (true)
            {
                var msgLength = reader.ReadUInt32();
                Log.Information($"Message received, length: {msgLength}");

                var msgBytes = reader.ReadBytes((int)msgLength);
                var msgJson = Encoding.UTF8.GetString(msgBytes);
                Log.Information($"{msgJson}");

                var baseMsg = JsonSerializer.Deserialize<BaseMessage>(msgJson, options);
                switch (baseMsg?.Type)
                {
                    case MessageType.ClientHello:
                    {
                        var hello = JsonSerializer.Deserialize<ClientHelloMessage>(msgJson, options);
                        Log.Information($"Client version: {hello?.ClientVersion}");
                        var serverHello = new ServerHelloMessage();
                        var serverHelloJson = JsonSerializer.Serialize(serverHello, options);
                        var serverHelloBytes = Encoding.UTF8.GetBytes(serverHelloJson);
                        var serverHelloLength = (uint)serverHelloBytes.Length;
                        writer.Write(serverHelloLength);
                        writer.Write(serverHelloBytes);
                        break;
                    }
                    case MessageType.RequestMappingList:
                    {
                        Log.Information("Sending file mapping table...");
                        var setMappings = new SetMappingListMessage();

                        var currentWorkingPath = Directory.GetCurrentDirectory();
                        foreach (var mod in ActiveMods)
                        {
                            foreach (var mapping in mod.Data?.Mappings ?? [])
                            {
                                var virtualPath = Path.Combine(currentWorkingPath, mapping.VirtualFile);
                                var realPath = Path.Combine(currentWorkingPath, ManagedModsPath, mod.Directory, mapping.RealFile);
                                setMappings.Mappings.Add(new Mapping
                                {
                                    VirtualPath = virtualPath,
                                    RealPath = realPath
                                });
                            }
                        }

                        var setMappingsJson = JsonSerializer.Serialize(setMappings, options);
                        Log.Information($"{setMappingsJson}");
                        var setMappingsBytes = Encoding.UTF8.GetBytes(setMappingsJson);
                        var setMappingsLength = (uint)setMappingsBytes.Length;
                        writer.Write(setMappingsLength);
                        writer.Write(setMappingsBytes);
                        break;
                    }
                    case null:
                    case MessageType.ServerHello:
                    case MessageType.SetMappingList:
                    default:
                        Log.Error("Invalid message type.");
                        break;
                }
            }
        }
        catch (IOException ex)
        {
            Log.Information($"Client disconnected: {ex.Message}");
        }
    }
}