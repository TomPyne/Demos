#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

enum class GltfComponentType : uint32_t
{
    BYTE = 5120,
    UNSIGNED_BYTE = 5121,
    SHORT = 5122,
    UNSIGNED_SHORT = 5123,
    UNSIGNED_INT = 5125,
    FLOAT = 5126,
};

enum class GltfElementType : uint32_t
{
    SCALAR,
    VEC2,
    VEC3,
    VEC4,
    MAT2,
    MAT3,
    MAT4,
};

struct GltfVec3
{
    double x, y, z;
    GltfVec3() = default;
    GltfVec3(double _x, double _y, double _z)
        :x(_x), y(_y), z(_z)
    {}
};

struct GltfVec4
{
    double x, y, z, w;
    GltfVec4() = default;
    GltfVec4(double _x, double _y, double _z, double _w)
        :x(_x), y(_y), z(_z), w(_w)
    {}
};

struct GltfMatrix
{
    double m[16];
    GltfMatrix()
    {
        m[0] = 1.0f; m[4] = 0.0f; m[8] = 0.0f; m[12] = 0.0f;
        m[1] = 0.0f; m[5] = 1.0f; m[9] = 0.0f; m[13] = 0.0f;
        m[2] = 0.0f; m[6] = 0.0f; m[10] = 1.0f; m[14] = 0.0f;
        m[3] = 0.0f; m[7] = 0.0f; m[11] = 0.0f; m[15] = 1.0f;
    }
};

struct GltfHdr
{
    uint32_t magic;
    uint32_t version;
    uint32_t length;
};

struct GltfChunk
{
    uint32_t length;
    uint32_t type;
};

struct GltfAsset
{
    std::string copyright;
    std::string generator;
    std::string version;
    std::string minVersion;
    // Gltf Unsupported: extensions
    // Gltf Unsupported: extras
};

struct GltfScene
{
    std::vector<uint32_t> nodes;
    std::string name;
    // Gltf Unsupported: extensions
    // Gltf Unsupported: extras
};
typedef std::vector<GltfScene> GltfSceneArray;


// A node in the node hierarchy.  When the node contains `skin`, all `mesh.primitives` **MUST** contain
// `JOINTS_0` and `WEIGHTS_0` attributes.  A node **MAY** have either a `matrix` or any combination of 
// `translation`/`rotation`/`scale` (TRS) properties. TRS properties are converted to matrices and 
// postmultiplied in the `T * R * S` order to compose the transformation matrix; first the scale is 
// applied to the vertices, then the rotation, and then the translation. If none are provided, the 
// transform is the identity. When a node is targeted for animation 
// (referenced by an animation.channel.target), `matrix` **MUST NOT** be present.",
struct GltfNode
{
    std::string name;
    int32_t mesh;
    GltfVec3 translation;
    GltfVec3 scale;
    GltfVec4 rotation;
    GltfMatrix matrix;
    std::vector<uint32_t> children;
    // Gltf Unsupported: camera
    // Gltf Unsupported: skin
    // Gltf Unsupported: weights
    // Gltf Unsupported: extensions
    // Gltf Unsupported: extras
};
typedef std::vector<GltfNode> GltfNodeArray;

struct GltfMeshAttribute
{
    std::string semantic;
    uint32_t index;
};
typedef std::vector<GltfMeshAttribute> GltfMeshAttributesArray;

enum class GltfMeshMode : uint32_t
{
    POINTS,
    LINES,
    LINE_LOOP,
    LINE_STRIP,
    TRIANGLES,
    TRIANGLE_STRIP,
    TRIANGLE_FAN,
};

struct GltfMeshPrimitive
{
    GltfMeshAttributesArray attributes;
    int32_t indices;
    int32_t material;
    GltfMeshMode mode;
    // Gltf Unsupported: targets
    // Gltf Unsupported: extensions
    // Gltf Unsupported: extras
};
typedef std::vector<GltfMeshPrimitive> GltfMeshPrimitivesArray;

struct GltfMesh
{
    std::string name;
    GltfMeshPrimitivesArray primitives;
    // Gltf Unsupported: weights
    // Gltf Unsupported: extensions
    // Gltf Unsupported: extras
};
typedef std::vector<GltfMesh> GltfMeshArray;

struct GltfTextureInfo
{
    uint32_t index;
    int32_t texcoord;
    // Gltf Unsupported: extensions
    // Gltf Unsupported: extras
};

struct GltfPbrMetallicRoughness
{
    GltfVec4 baseColorFactor;
    bool hasBaseColorTexture;
    GltfTextureInfo baseColorTexture;
    float metallicFactor;
    float roughnessFactor;
    bool hasMetallicRoughnessTexture;
    GltfTextureInfo metallicRoughnessTexture;
    // Gltf Unsupported: extensions
    // Gltf Unsupported: extras
};

struct GltfNormalTextureInfo
{
    uint32_t index;
    int32_t texcoord;
    // Gltf Unsupported: scale
};

#undef OPAQUE

enum class GltfAlphaMode : uint8_t
{
    OPAQUE,
    MASK,
    BLEND,
};

struct GltfMaterial
{
    std::string name;
    GltfPbrMetallicRoughness pbr;
    bool hasNormalTexture;
    GltfNormalTextureInfo normalTexture;
    GltfAlphaMode alphaMode;
    float alphaCutoff;
    bool doubleSided;
    // Gltf Unsupported: occlusionTexture
    // Gltf Unsupported: emissiveTexture
    // Gltf Unsupported: emissiveFactor
    // Gltf Unsupported: extensions
    // Gltf Unsupported: extras
};
typedef std::vector<GltfMaterial> GltfMaterialArray;

struct GltfTexture
{
    std::string name;
    int32_t sampler;
    int32_t source;
    // Gltf Unsupported: extensions
    // Gltf Unsupported: extras
};
typedef std::vector<GltfTexture> GltfTextureArray;

struct GltfSampler
{
    std::string name;
    int32_t magFilter;
    int32_t minFilter;
    int32_t wrapS;
    int32_t wrapT;
    // Gltf Unsupported: extensions
    // Gltf Unsupported: extras
};
typedef std::vector<GltfSampler> GltfSamplerArray;

struct GltfImage
{
    std::string name;
    std::string uri;
    std::string mimeType;
    int32_t bufferView;
    // Gltf Unsupported: extensions
    // Gltf Unsupported: extras
};
typedef std::vector<GltfImage> GltfImageArray;

struct GltfAccessor
{
    std::string name;
    int32_t bufferView;
    int32_t byteOffset;
    GltfComponentType componentType;
    bool normalized;
    int32_t count;
    GltfElementType type;
    double max[16];
    double min[16];
    // Gltf Unsupported: sparse
    // Gltf Unsupported: extensions
    // Gltf Unsupported: extras
};
typedef std::vector<GltfAccessor> GltfAccessorArray;

struct GltfBufferView
{
    std::string name;
    int32_t buffer;
    int32_t byteOffset;
    int32_t byteLength;
    int32_t byteStride;
    int32_t target;
    // Gltf Unsupported: extensions
    // Gltf Unsupported: extras
};
typedef std::vector<GltfBufferView> GltfBufferViewArray;

struct GltfBuffer
{
    std::string name;
    std::string uri;
    int32_t byteLength;
    // Gltf Unsupported: extensions
    // Gltf Unsupported: extras
};
typedef std::vector<GltfBuffer> GltfBufferArray;

struct Gltf
{
    std::vector<std::string> extensionsUsed;
    std::vector<std::string> extensionsRequired;
    GltfAccessorArray accessors;
    GltfAsset asset;
    GltfBufferArray buffers;
    GltfBufferViewArray bufferViews;
    GltfImageArray images;
    GltfMaterialArray materials;
    GltfMeshArray meshes;
    GltfNodeArray nodes;
    GltfSamplerArray samplers;
    int32_t scene;
    GltfSceneArray scenes;
    GltfTextureArray textures;
    // Gltf Unsupported: animations
    // Gltf Unsupported: cameras
    // Gltf Unsupported: skins
    // Gltf Unsupported: extensions
    // Gltf Unsupported: extras

    std::unique_ptr<uint8_t[]> data;
};

bool GltfLoader_Load(const char* path, Gltf* loadedGltf);
size_t GltfLoader_SizeOfComponent(GltfComponentType ct);
size_t GltfLoader_ComponentCount(GltfElementType et);