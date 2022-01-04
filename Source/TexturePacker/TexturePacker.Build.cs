namespace UnrealBuildTool.Rules
{
	public class TexturePacker : ModuleRules
	{
		public TexturePacker(ReadOnlyTargetRules Target) : base(Target)
		{
			DefaultBuildSettings = BuildSettingsVersion.V2;
			CppStandard = CppStandardVersion.Cpp17;
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
			
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
				}
			);
		}
	}
}
