
std::string replaceSpacesWithUnderscores(std::string in) {
  std::replace(in.begin(), in.end(), ' ', '_');
  return in;
}

std::string vs_generateUUID() {
  static size_t count = 0;
  ++count;

  char buffer[64];
  sprintf(buffer, "00000000-0000-0000-0000-%012zi", count);
  return buffer;
};

std::string solutionPlatform2String(EPlatformType platform) {
  switch (platform) {
    case EPlatformTypeX86:
      return "x86";
    case EPlatformTypeX64:
      return "x64";
    case EPlatformTypeARM:
      return "ARM";
  }
  return "";
}

void vs2019_generateInFolder(const char* workspace_path) {
  int count_folder_depth = 1;
  {
    const char* c = workspace_path;
    while (*c) {
      if (*c == '/') count_folder_depth += 1;
      ++c;
    }
    if (*(c - 1) == '/') count_folder_depth -= 1;
  }

  int result = make_folder(workspace_path);
  if (result != 0) {
    fprintf(stderr, "Error %i creating path '%s'\n", result, workspace_path);
  }
  (void)chdir(workspace_path);

  std::string workspace_file_path = privateData.workspaceLabel;
  workspace_file_path += ".sln";
  FILE* workspace = fopen(workspace_file_path.c_str(), "wb");
  if (workspace == NULL) {
    fprintf(stderr, "Couldn't create workspace.sln\n");
    return;
  }

  fprintf(workspace, R"lit(Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 16
VisualStudioVersion = 16.0.29709.97
MinimumVisualStudioVersion = 10.0.40219.1
)lit");

  std::vector<std::string> project_ids(privateData.projects.size());

  for (size_t i = 0; i < privateData.projects.size(); ++i) {
    project_ids[i] = vs_generateUUID();
    auto projectId = project_ids[i];
    auto p         = privateData.projects[i];
    fprintf(workspace,
            "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"%s\", \"%s.vcxproj\", "
            "\"{%s}\"\n",
            p->name, replaceSpacesWithUnderscores(p->name).c_str(), projectId.c_str());
    fprintf(workspace, "EndProject\n");
  }

  fprintf(workspace, R"lit(Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
)lit");
  for (unsigned ci = 0; ci < privateData.configurations.size(); ++ci) {
    auto c = privateData.configurations[ci]->label;
    for (unsigned pi = 0; pi < privateData.platforms.size(); ++pi) {
      auto p = privateData.platforms[pi]->type;
      fprintf(workspace, "\t\t%s|%s = %s|%s\n", c, solutionPlatform2String(p).c_str(), c,
              solutionPlatform2String(p).c_str());
    }
  }
  fprintf(workspace, R"lit(	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
)lit");
  for (size_t i = 0; i < privateData.projects.size(); ++i) {
    auto projectId = project_ids[i];
    for (unsigned ci = 0; ci < privateData.configurations.size(); ++ci) {
      auto c = privateData.configurations[ci]->label;
      for (unsigned pi = 0; pi < privateData.platforms.size(); ++pi) {
        auto p = privateData.platforms[pi]->type;
        fprintf(workspace, "\t\t{%s}.%s|%s.ActiveCfg = %s|%s\n", projectId.c_str(), c,
                solutionPlatform2String(p).c_str(), c, projectPlatform2String(p).c_str());
        fprintf(workspace, "\t\t{%s}.%s|%s.Build.0 = %s|%s\n", projectId.c_str(), c,
                solutionPlatform2String(p).c_str(), c, projectPlatform2String(p).c_str());
      }
    }
  }

  fprintf(workspace, R"lit(	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
	GlobalSection(ExtensibilityGlobals) = postSolution
		SolutionGuid = {7354F2AC-FB49-4B5D-B080-EDD798F580A5}
	EndGlobalSection
EndGlobal
)lit");

  fclose(workspace);
  printf("Created workspace at '%s'\n", workspace_path);

  for (unsigned i = 0; i < privateData.projects.size(); ++i) {
    auto p          = privateData.projects[i];
    auto project_id = project_ids[i];
    vs2019_createProjectFile(p, project_id.c_str(), project_ids, count_folder_depth);
    vs2019_createFilters(p);

    printf("Created project '%s' at '%s'\n", p->name, workspace_path);
  }
}
