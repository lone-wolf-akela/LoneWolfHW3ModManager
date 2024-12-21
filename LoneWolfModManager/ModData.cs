namespace LoneWolfModManager;

public class ModFileMapping
{
    public string VirtualFile { get; set; } = "";
    public string RealFile { get; set; } = "";
}

public class ModData
{
    public int ModManagerVersion { get; set; }
    public string ModName { get; set; } = "";
    public string ModVersion { get; set; } = "";
    public string ModAuthor { get; set; } = "";
    public string? PreviewImage { get; set; }
    public List<ModFileMapping> Mappings { get; set; } = [];
}