std::string replaceSpacesWithUnderscores(std::string in)
{
    std::replace(in.begin(), in.end(), ' ', '_');
    return in;
}

const char *strip_path(const char *path)
{
    return strrchr(path, '/') + 1;
}
