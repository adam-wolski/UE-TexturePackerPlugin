namespace UnrealBuildTool.Rules
{
	public class TexturePacker : ModuleRules
	{
		public TexturePacker(ReadOnlyTargetRules Target) : base(Target)
		{
			DefaultBuildSettings = BuildSettingsVersion.V2;
			CppStandard = CppStandardVersion.Cpp17;
			PCHUsage = PCHUsageMode.NoSharedPCHs;
			PrivatePCHHeaderFile = "Private/TexturePackerPCH.h";

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"InputCore",
					"Slate",
					"SlateCore",
					"UnrealEd",
				}
			);
		}
	}
}
