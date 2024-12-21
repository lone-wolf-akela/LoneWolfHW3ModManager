# What's this?
This is a mod manager for the game Homeworld 3.

Imagine you have 10 different mods. You can't simply put all of them in your game folder because the game will load all the mods simultaneously, and they might not be compatible with each other. You might think, "Okay, I'll just manage them manually," but you'll soon find that each mod contains multiple files stored in different paths (some in `~mod`, some in `mods`, etc.), making manual management a real headache.

This mod manager is designed to help with this situation. With it, you can install multiple mods at once and enable or disable any of them with a single click.

# How to Use

[中文使用说明](#使用说明)

(All paths correspond to the default Steam installation directory. Remember to adjust them according to the actual game path on your computer.)

1. Place `LoneWolfModManager.exe` in `C:\Program Files (x86)\Steam\steamapps\common\Homeworld 3`.
2. Put your mods in the `C:\Program Files (x86)\Steam\steamapps\common\Homeworld 3\ManagedMods` folder, which will be created the first time you open `LoneWolfModManager.exe`. If your mod does not include a `mod.json` file, you will need to create one yourself. See the **Directory Structure & Mod Info File** section for details.
3. Place `winmm.dll` in `C:\Program Files (x86)\Steam\steamapps\common\Homeworld 3\Homeworld3\Binaries\Win64`.
4. Launch `LoneWolfModManager.exe` and select the mod you want to use.
5. Keep `LoneWolfModManager.exe` open and start the game normally using Steam/Epic/other platforms.
6. The mod should load correctly.

# Directory Structure & Mod Info File

Mods need to follow a directory structure like the example below to be loaded by this mod manager:

```
ManagedMods
├── Mod1 (any name will work)
│   ├── mod.json
│   ├── preview_image.png (optional, can have other names, and other common image formats such as JPEG are also supported)
│   ├── other_mod_files
│   ├── other_mod_folders
│   │   └── more_layers_of_folders_are_also_supported
├── Mod2
│   ├── mod.json
│   └── ...
```

The key is to have a separate folder for each mod, with a `mod.json` file inside each mod's folder.

The `mod.json` file is a text file. An example is shown below:

```json
{
  "mod_manager_version": 1,
  "mod_name": "FX3Demo",
  "mod_version": "0.0.2",
  "mod_author": "9CCN",
  "preview_image": "fx-demo.png",
  "mappings": [
    {
      "virtual_file": "Homeworld3/Content/Paks/~mods/FXMOD_P.pak",
      "real_file": "FXMOD_P.pak"
    },
    {
      "virtual_file": "Homeworld3/mods/FXMOD/FXMOD.uplugin",
      "real_file": "FXMOD/FXMOD.uplugin"
    },
    {
      "virtual_file": "Homeworld3/mods/FXMOD/Content/Paks/WindowsNoEditor/FXMODpakchunk0-WindowsNoEditor.pak",
      "real_file": "FXMOD/Content/Paks/WindowsNoEditor/FXMODpakchunk0-WindowsNoEditor.pak"
    }
  ]
}
```

The main functionality is in the `mappings` section. For each `virtual_file` and `real_file` pair, the `real_file` points to an existing file within the mod's folder inside `ManagedMods`, while `virtual_file` points to a non-existing path. When the game tries to access the `virtual_file`, the mod manager redirects it to the `real_file` (if that mod is selected in the mod manager GUI).



# 使用说明

（所有路径对应默认的 Steam 安装目录。请根据你电脑上的实际游戏路径进行调整。）

1. 将 `LoneWolfModManager.exe` 放置在 `C:\Program Files (x86)\Steam\steamapps\common\Homeworld 3`。
2. 将你的模组放在 `C:\Program Files (x86)\Steam\steamapps\common\Homeworld 3\ManagedMods` 文件夹中，该`ManagedMods`文件夹将在你第一次打开 `LoneWolfModManager.exe` 时创建。如果你的模组不包含 `mod.json` 文件，你需要自行创建。详情请参阅**目录结构和模组信息文件**部分。
3. 将 `winmm.dll` 放置在 `C:\Program Files (x86)\Steam\steamapps\common\Homeworld 3\Homeworld3\Binaries\Win64`。
4. 启动 `LoneWolfModManager.exe` 并选择你想使用的模组。
5. 保持 `LoneWolfModManager.exe` 打开状态，并通过 Steam/Epic/其他平台正常启动游戏。
6. 你的模组应已经被游戏正确加载了。

# 目录结构和模组信息文件

模组需要遵循如下示例的目录结构，才能被此模组管理器加载：

```
ManagedMods
├── Mod1 (任意名称均可)
│   ├── mod.json
│   ├── preview_image.png (可选，文件名任意，也支持 JPEG 等其他常见图像格式)
│   ├── 其他Mod文件
│   ├── 其他Mod文件夹
│   │   └── 更多层级的文件夹也是支持的
├── Mod2
│   ├── mod.json
│   └── ...
```

关键是为每个模组创建一个单独的文件夹，并在每个模组的文件夹中包含一个 `mod.json` 文件。

`mod.json` 文件是一个文本文件。示例如下：

```json
{
  "mod_manager_version": 1,
  "mod_name": "FX3Demo",
  "mod_version": "0.0.2",
  "mod_author": "9CCN",
  "preview_image": "fx-demo.png",
  "mappings": [
    {
      "virtual_file": "Homeworld3/Content/Paks/~mods/FXMOD_P.pak",
      "real_file": "FXMOD_P.pak"
    },
    {
      "virtual_file": "Homeworld3/mods/FXMOD/FXMOD.uplugin",
      "real_file": "FXMOD/FXMOD.uplugin"
    },
    {
      "virtual_file": "Homeworld3/mods/FXMOD/Content/Paks/WindowsNoEditor/FXMODpakchunk0-WindowsNoEditor.pak",
      "real_file": "FXMOD/Content/Paks/WindowsNoEditor/FXMODpakchunk0-WindowsNoEditor.pak"
    }
  ]
}
```

关键内容在于 `mappings` 部分。对于每一对 `virtual_file` 和 `real_file`，`real_file` 指向 `ManagedMods` 中模组文件夹内的真实存在的文件，而 `virtual_file` 指向一个不存在的路径。当游戏尝试访问 `virtual_file` 时，模组管理器会将其重定向到 `real_file`（当然，只有该模组在模组管理器 GUI 中被选中时才会重定向）。
