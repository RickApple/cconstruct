std::string replaceSpacesWithUnderscores(std::string in)
{
    std::replace(in.begin(), in.end(), ' ', '_');
    return in;
}
