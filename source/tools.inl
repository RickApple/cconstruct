#if defined(_MSC_VER)
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

std::string replaceSpacesWithUnderscores(std::string in)
{
    std::replace(in.begin(), in.end(), ' ', '_');
    return in;
}

const char *strip_path(const char *path)
{
    return strrchr(path, '/') + 1;
}

#if defined(_MSC_VER)
int make_folder(const char *folder_path)
{
}
#else
int make_folder(const char *folder_path)
{
    char buffer[1024] = {0};

    const char *next_sep = folder_path;
    while ((next_sep = strchr(next_sep, '/')) != NULL)
    {
        strncpy(buffer, folder_path, next_sep - folder_path);
        int result = mkdir(buffer, 0777);
        if (result != 0)
        {
            if (errno != EEXIST)
                return errno;
        }
        next_sep += 1;
    }
    int result = mkdir(folder_path, 0777);
    if (result != 0)
    {
        if (errno != EEXIST)
            return errno;
    }
    return 0;
}
#endif
