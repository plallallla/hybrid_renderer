#include "obj_loader.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace hr
{
namespace
{
struct ObjIndex
{
    int position = 0;
    int texcoord = 0;
    int normal = 0;
};

ObjIndex ParseIndex(const std::string& token)
{
    ObjIndex index;
    std::stringstream ss(token);
    std::string part;

    std::getline(ss, part, '/');
    if (!part.empty())
    {
        index.position = std::stoi(part);
    }

    if (std::getline(ss, part, '/'))
    {
        if (!part.empty())
        {
            index.texcoord = std::stoi(part);
        }
    }

    if (std::getline(ss, part, '/'))
    {
        if (!part.empty())
        {
            index.normal = std::stoi(part);
        }
    }

    return index;
}

int FixIndex(int index, int size)
{
    if (index > 0)
    {
        return index - 1;
    }
    if (index < 0)
    {
        return size + index;
    }
    return 0;
}
} // namespace

bool LoadObj(const std::string& path, Mesh& mesh)
{
    std::ifstream file(path);
    if (!file)
    {
        return false;
    }

    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> texcoords;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::stringstream ss(line);
        std::string tag;
        ss >> tag;

        if (tag == "v")
        {
            Vec3 v{};
            ss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        }
        else if (tag == "vn")
        {
            Vec3 n{};
            ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (tag == "vt")
        {
            Vec2 uv{};
            ss >> uv.x >> uv.y;
            texcoords.push_back(uv);
        }
        else if (tag == "f")
        {
            std::vector<ObjIndex> face;
            std::string token;
            while (ss >> token)
            {
                face.push_back(ParseIndex(token));
            }

            if (face.size() < 3)
            {
                continue;
            }

            const size_t baseVertex = mesh.vertices.size();
            for (const ObjIndex& index : face)
            {
                Vertex vertex{};
                const int positionIndex = FixIndex(index.position, static_cast<int>(positions.size()));
                vertex.position = positions[static_cast<size_t>(positionIndex)];

                if (!normals.empty() && index.normal != 0)
                {
                    const int normalIndex = FixIndex(index.normal, static_cast<int>(normals.size()));
                    vertex.normal = normals[static_cast<size_t>(normalIndex)];
                }
                else
                {
                    vertex.normal = {0.0f, 0.0f, 1.0f};
                }

                if (!texcoords.empty() && index.texcoord != 0)
                {
                    const int texcoordIndex = FixIndex(index.texcoord, static_cast<int>(texcoords.size()));
                    vertex.uv = texcoords[static_cast<size_t>(texcoordIndex)];
                }
                else
                {
                    vertex.uv = {0.0f, 0.0f};
                }

                mesh.vertices.push_back(vertex);
            }

            for (size_t i = 1; i + 1 < face.size(); ++i)
            {
                mesh.triangles.push_back({
                    static_cast<int>(baseVertex),
                    static_cast<int>(baseVertex + i),
                    static_cast<int>(baseVertex + i + 1),
                });
            }
        }
    }

    return !mesh.vertices.empty() && !mesh.triangles.empty();
}
} // namespace hr
