/*
**==============================================================================
**
** Copyright (c) 2008, Michael E. Brasher
** 
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and associated documentation files (the "Software"),
** to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense,
** and/or sell copies of the Software, and to permit persons to whom the
** Software is furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
**
**==============================================================================
*/

#include "konkret/konkret.h"
#include <unistd.h>
#include <vector>
#include <string>

using namespace std;

static const char* arg0;

static vector<string> ns1;
static vector<string> ns2;

static bool r_opt;

struct Reg
{
    string nameSpace;
    string className;
    string providerName;
    string types;
};

static string escape_namespace(const string& ns)
{
    string result;

    for (size_t i = 0; i < ns.size(); i++)
    {
        if (ns[i] == ' ')
            result += '\\';
        result += ns[i];
    }

    return result;
}

int main(int argc, char** argv)
{
    arg0 = argv[0];
    const char USAGE[] =
        "Usage: %s [OPTIONS] PROVIDER_LIBRARY\n"
        "\n"
        "SYNOPSIS:\n"
        "  Generates an SFCB-style .reg file by default\n"
        "\n"
        "OPTIONS:\n"
        "  -n NS1=NS2   -- register providers marked for namespace NS1 into\n"
        "                  namespace NS2 instead. For example, to register\n"
        "                  providers containing 'root/cimv2' to 'root/xxx',\n"
        "                  use '-n root/cimv2=root/xxx'.\n"
        "  -r           -- Generates simpler .registration file format instead.\n"
        "  -h           -- Print this help message\n"
        "  -v           -- Print version\n"
        "\n";


    // Process command-line options.

    for (int opt; (opt = getopt(argc, argv, "hvn:r")) != -1; )
    {
        switch (opt)
        {
            case 'h':
                printf((char*)USAGE, arg0);
                exit(0);
                break;

            case 'v':
                printf("%s\n", KONKRET_VERSION_STRING);
                exit(0);
                break;

            case 'r':
                r_opt = true;
                break;

            case 'n':
            {
                char* p = strchr(optarg, '=');

                if (!p)
                {
                    fprintf(stderr, "%s: invalid -n option argument: '%s' "
                        "(required '=' character missing).\n", arg0, optarg);
                    exit(1);
                }

                ns1.push_back(string(optarg, p - optarg));
                ns2.push_back(p + 1);
                break;
            }

            default:
            {
                fprintf(stderr, "%s: invalid option: %c; try -h for help\n", 
                    arg0, opt);
                exit(1);
                break;
            }
        }
    }

    // Check for one argument:

    argv += optind;
    argc -= optind;

    if (argc != 1)
    {
        fprintf(stderr, "%s: insufficient arguments. Try -h for help\n", arg0);
        exit(1);
    }

    const char* path = argv[0];

    // Read library file into memory.

    vector<char> data;
    {
        FILE* fp = fopen(path, "rb");
        
        if (!fp)
        {
            fprintf(stderr, "%s: failed to open file: %s\n", arg0, path);
            exit(1);
        }

        // Read the file into memory.

        char buf[1024];
        size_t n;

        while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
            data.insert(data.end(), buf, buf + n);

        data.push_back('\0');

        // Close the file:

        fclose(fp);
    }

    // Search for registration strings:

    // Search for version string.

    const char* p = &data[0];
    size_t n = data.size();
    const char REG[] = "@(#)KONKRET_REGISTRATION=";

    std::vector<Reg> regs;

    while (n >= sizeof(REG)-1)
    {
        if (memcmp(p, REG, sizeof(REG)-1) == 0)
        {
            char buf[4096];

            size_t r = strlen(p) + 1;
            *buf = '\0';
            strncat(buf, p + sizeof(REG)-1, sizeof(buf)-1);

            Reg reg;

            // Get nameSpace:

            char* q = strtok(buf, ":");

            if (q)
                reg.nameSpace = q;

            // Get className:

            if (q && (q = strtok(NULL, ":")))
                reg.className = q;

            // Get providerName:

            if (q && (q = strtok(NULL, ":")))
                reg.providerName = q;

            // Get types:

            if (q && (q = strtok(NULL, ":")))
                reg.types = q;

            regs.push_back(reg);

            p += r;
            n -= r;
        }
        else
        {
            p++;
            n--;
        }
    }

    // Extract location name:
    string location;
    {
        string s = path;

        // Remove leading path:

        size_t r = s.rfind('/');

        if (r != string::npos)
            s = s.substr(r+1);

        // Remove "lib" prefix:
        
        if (s.substr(0, 3) != "lib")
        {
            fprintf(stderr, "%s: library names must start with \"lib\": %s\n",
                arg0, path);
            exit(1);
        }

        s = s.substr(3);

        // Remove extension:

        s = s.substr(0, s.find('.'));
        location = s;
    }

    // Perform namespace substitutions:

    for (size_t i = 0; i < regs.size(); i++)
    {
        for (size_t j = 0; j < ns1.size(); j++)
        {
            if (regs[i].nameSpace == ns1[j])
                regs[i].nameSpace = ns2[j];
        }
    }

    // Write out the file:

    for (size_t i = 0; i < regs.size(); i++)
    {
        if (r_opt)
        {
            printf("%s %s %s %s %s\n", 
                regs[i].className.c_str(),
                escape_namespace(regs[i].nameSpace).c_str(),
                regs[i].providerName.c_str(),
                location.c_str(),
                regs[i].types.c_str());
        }
        else
        {
            printf(
                "[%s]\n"
                "   provider: %s\n"
                "   location: %s\n"
                "   type: %s\n"
                "   namespace: %s\n"
                "#\n",
                regs[i].className.c_str(),
                regs[i].providerName.c_str(),
                location.c_str(),
                regs[i].types.c_str(),
                regs[i].nameSpace.c_str());
        }
    }

    // Search for "@(#)KONKRET_REGISTRATION" strings:

    return 0;
}
