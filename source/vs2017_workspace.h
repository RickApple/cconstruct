void addConfiguration( const char* in_configuration_name ) {
  privateData.configurations.push_back( in_configuration_name );
}
void addPlatform( const char* in_platform_name, EPlatformType in_type ) {
  privateData.platform_names.push_back( in_platform_name );
  privateData.platforms.push_back( in_type );
}

std::string replaceSpacesWithUnderscores( std::string in ) {
  std::replace( in.begin(), in.end(), ' ', '_' );
  return in;
}

void generate() {
  std::string workspace_file_path = privateData.outputFolder;

  workspace_file_path += "/";
  workspace_file_path += "workspace.sln";
  FILE* workspace = fopen( workspace_file_path.c_str(), "wb" );
  if( workspace ) {

    const char* sln = R"lit(Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio 15
VisualStudioVersion = 15.0.27428.2015
MinimumVisualStudioVersion = 10.0.40219.1
)lit";
    fwrite( sln, 1, strlen( sln ), workspace );

    const std::string projectId = "{4470604D-2B04-466E-A39B-9E49BA6DA261}";
    for( auto p : privateData.projects ) {
      std::string pt = "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"" + p->name + "\", \"" +
                       replaceSpacesWithUnderscores( p->name ) +
                       ".vcxproj\", "
                       "\"" +
                       projectId + "\"";
      fwrite( pt.c_str(), 1, pt.length(), workspace );
    }
    writeToFile( workspace, R"lit(
EndProject
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
)lit" );
    for( unsigned ci = 0; ci < privateData.configurations.size(); ++ci ) {
      auto c = privateData.configurations[ci];
      for( unsigned pi = 0; pi < privateData.platforms.size(); ++pi ) {
        auto        p = privateData.platforms[pi];
        std::string s = "\t\t";
        s += c;
        s += "|";
        s += p;
        s += " = ";
        s += c;
        s += "|";
        s += p;
        s += "\n";
        writeToFile( workspace, s.c_str() );
      }
    }
    writeToFile( workspace, R"lit(	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
)lit" );
    for( unsigned ci = 0; ci < privateData.configurations.size(); ++ci ) {
      auto c = privateData.configurations[ci];
      for( unsigned pi = 0; pi < privateData.platforms.size(); ++pi ) {
        auto        p = privateData.platforms[pi];
        std::string s = "\t\t";
        s += projectId;
        s += ".";
        s += c;
        s += "|";
        s += p;
        s += ".ActiveCfg = ";
        s += c;
        s += "|";
        s += p;
        s += "\n";
        writeToFile( workspace, s.c_str() );
        s = "\t\t";
        s += projectId;
        s += ".";
        s += c;
        s += "|";
        s += p;
        s += ".Build.0 = ";
        s += c;
        s += "|";
        s += p;
        s += "\n";
        writeToFile( workspace, s.c_str() );
      }
    }

    writeToFile( workspace, R"lit(	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
	GlobalSection(ExtensibilityGlobals) = postSolution
		SolutionGuid = {7354F2AC-FB49-4B5D-B080-EDD798F580A5}
	EndGlobalSection
EndGlobal
)lit" );

    fclose( workspace );
    printf( "Created workspace at '%s'\n", privateData.outputFolder );

    for( unsigned i = 0; i < privateData.projects.size(); ++i ) {
      auto p = privateData.projects[i];
      createProjectFile( p );
    }
  } else {
    return exit( 1 );
  }
}

void setOutputFolder( const char* of ) { privateData.outputFolder = of; }

void addProject( const void* in_project ) { privateData.projects.push_back( (TProject*)in_project ); }


TBuilder builder = {
    {
        ::createProject,
        ::addFileToProject,
    },
    {
        ::setOutputFolder,
        ::addProject,
        ::addConfiguration,
        ::addPlatform,
    },
    ::generate
};