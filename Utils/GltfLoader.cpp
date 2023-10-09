#include "GltfLoader.h"

#include "Files.h"
#include "Logging.h"

#include <ThirdParty/rapidjson/document.h>

#include <memory>
#include <string>
#include <vector>

#define GLTF_LOG_LEVEL_VERBOSE 2
#define GLTF_LOG_LEVEL_WARNING 1
#define GLTF_LOG_LEVEL_ERROR 0
#define GLTF_LOG_LEVEL_OFF -1

#define GLTF_LOG_LEVEL (GLTF_LOG_LEVEL_VERBOSE)

#define GLTF_LOG_ENABLED(x) (x <= GLTF_LOG_LEVEL)

constexpr uint32_t GltfMagic = 0x46546c67;
constexpr uint32_t GltfJsonChunk = 0x4e4f534a;
constexpr uint32_t GltfBinChunk = 0x004e4942;

template<typename GltfArray>
static bool GltfArray_Parse(const rapidjson::Value& json, GltfArray* arr);

static bool EnsureSupport(const rapidjson::Value& json, const char* type, const char* member)
{
#if GLTF_LOG_ENABLED(GLTF_LOG_LEVEL_VERBOSE)
    return ENSUREMSG(!json.HasMember(member), "Gltf: %s has %s and isn't supported", type, member);
#else
    return true;
#endif
}

static bool EnsureHas(const rapidjson::Value& json, const char* type, const char* member)
{
#if GLTF_LOG_ENABLED(GLTF_LOG_LEVEL_VERBOSE)
    return ENSUREMSG(json.HasMember(member), "Gltf: %s missing %s", type, member);
#else
    return true;
#endif
}

static inline void CheckGltfSupport(const rapidjson::Value& json, const char* type, const char* member)
{
#if GLTF_LOG_ENABLED(GLTF_LOG_LEVEL_VERBOSE)
    if (json.HasMember(member)) LOGWARNING("Gltf: '%s' has unsupported member '%s'", type, member);
#endif
}

template<typename T> 
T Gltf_ParseType(const rapidjson::Value& json)
{
    return json.Get<T>();
}

template<>
std::string Gltf_ParseType(const rapidjson::Value& json)
{
    return json.GetString();
}

template<>
GltfVec3 Gltf_ParseType(const rapidjson::Value& json)
{
    ASSERTMSG(json.IsArray(), "Gltf_ParseType<>GltfVec3: Json value is not array type");
    ASSERTMSG(json.Size() >= 3, "Gltf_ParseType<GltfVec3>: Json value is not correct size (3)");

    return { json[0].GetDouble(), json[1].GetDouble(), json[2].GetDouble() };
}

template<>
GltfVec4 Gltf_ParseType(const rapidjson::Value& json)
{
    ASSERTMSG(json.IsArray(), "Gltf_ParseType<GltfVec4>: Json value is not array type");
    ASSERTMSG(json.Size() >= 4, "Gltf_ParseType<GltfVec4>: Json value is not correct size (4)");

    return { json[0].GetDouble(), json[1].GetDouble(), json[2].GetDouble(), json[3].GetDouble() };
}

template<>
GltfMatrix Gltf_ParseType(const rapidjson::Value& json)
{
    ASSERTMSG(json.IsArray(), "Gltf_ParseType<GltfMatrix>: Json value is not array type");
    ASSERTMSG(json.Size() >= 16, "Gltf_ParseType<GltfMatrix>: Json value is not correct size (16)");

    GltfMatrix matrix{};
    for (uint32_t i = 0; i < 16; i++)
        matrix.m[i] = json[i].GetDouble();

    return matrix;
}

static bool Gltf_Parse(const rapidjson::Value& json, GltfAsset* asset)
{
    if(!EnsureHas(json, "GltfAsset", "version")) return false;

    CheckGltfSupport(json, "GltfAsset", "extensions");
    CheckGltfSupport(json, "GltfAsset", "extras");

    asset->version = json["version"].GetString();

    asset->copyright = json.HasMember("copyright") ? json["copyright"].GetString() : "";
    asset->generator = json.HasMember("generator") ? json["generator"].GetString() : "";
    asset->minVersion = json.HasMember("minVersion") ? json["minVersion"].GetString() : "";

    return true;
}

static bool Gltf_Parse(const rapidjson::Value& json, GltfScene* scene)
{
    CheckGltfSupport(json, "GltfAsset", "extensions");
    CheckGltfSupport(json, "GltfAsset", "extras");

    scene->name = json.HasMember("name") ? json["name"].GetString() : "";
    if (json.HasMember("nodes"))
    {
        scene->nodes.reserve(json["nodes"].Size());
        for (const rapidjson::Value& v : json["nodes"].GetArray())
            scene->nodes.push_back(v.GetUint());
    }

    return true;
}

template<typename T>
T Gltf_JsonGet(const rapidjson::Value& json, const char* name, const T& defaultValue = {})
{
    return json.HasMember(name) ? Gltf_ParseType<T>(json[name]) : defaultValue;
}

static bool Gltf_Parse(const rapidjson::Value& json, GltfNode* node)
{
    CheckGltfSupport(json, "GltfNode", "camera");
    CheckGltfSupport(json, "GltfNode", "skin");
    CheckGltfSupport(json, "GltfNode", "weights");
    CheckGltfSupport(json, "GltfNode", "extensions");
    CheckGltfSupport(json, "GltfNode", "extras");

    node->name =           Gltf_JsonGet<std::string>(json, "name");
    node->mesh =           Gltf_JsonGet<int>(json, "mesh", -1);
    node->translation =    Gltf_JsonGet<GltfVec3>(json, "translation");
    node->scale =          Gltf_JsonGet<GltfVec3>(json, "scale", GltfVec3{ 1.0, 1.0, 1.0 });
    node->rotation =       Gltf_JsonGet<GltfVec4>(json, "rotation", GltfVec4{ 0.0, 0.0, 0.0, 1.0 });
    node->matrix =         Gltf_JsonGet<GltfMatrix>(json, "matrix");

    if (json.HasMember("children"))
    {
        node->children.reserve(json["children"].Size());
        for (const rapidjson::Value& v : json["children"].GetArray())
            node->children.push_back(v.GetUint());
    }

    return true;
}

static GltfMeshAttributesArray GltfMeshAttributes_Parse(const rapidjson::Value& json)
{
    GltfMeshAttributesArray attributes;
    for (const auto& m : json.GetObject())
    {
        attributes.push_back({});
        attributes.back().semantic = m.name.GetString();
        attributes.back().index = m.value.GetInt();
    }

    return attributes;
}

static bool Gltf_Parse(const rapidjson::Value& json, GltfMeshPrimitive* primitive)
{
    if (!EnsureHas(json, "GltfMeshPrimitive", "attributes")) return false;

    CheckGltfSupport(json, "GltfMeshPrimitive", "targets");
    CheckGltfSupport(json, "GltfMeshPrimitive", "extensions");
    CheckGltfSupport(json, "GltfMeshPrimitive", "extras");

    primitive->attributes = GltfMeshAttributes_Parse(json["attributes"]);
    primitive->indices = json.HasMember("indices") ? json["indices"].GetInt() : -1;
    primitive->material = json.HasMember("material") ? json["material"].GetInt() : -1;
    primitive->mode = (GltfMeshMode)Gltf_JsonGet<int>( json, "mode", (int)GltfMeshMode::TRIANGLES);

    return true;
}

static bool Gltf_Parse(const rapidjson::Value& json, GltfMesh* mesh)
{
    if (!EnsureHas(json, "GltfMesh", "primitives")) return false;

    CheckGltfSupport(json, "GltfMesh", "weights");
    CheckGltfSupport(json, "GltfMesh", "extensions");
    CheckGltfSupport(json, "GltfMesh", "extras");

    if (!GltfArray_Parse(json["primitives"], &mesh->primitives)) return false;       

    mesh->name = json.HasMember("name") ? json["name"].GetString() : "";

    return true;
}

static bool GltfTextureInfo_Parse(const rapidjson::Value& json, GltfTextureInfo* textureInfo)
{
    if (!EnsureHas(json, "GltfTextureInfo", "index")) return false;
    
    CheckGltfSupport(json, "GltfTextureInfo", "extensions");
    CheckGltfSupport(json, "GltfTextureInfo", "extras");

    textureInfo->index = json["index"].GetInt();
    textureInfo->texcoord = json.HasMember("texCoord") ? json["texCoord"].GetInt() : -1;

    return true;
}

static bool GltfNormalTextureInfo_Parse(const rapidjson::Value& json, GltfNormalTextureInfo* textureInfo)
{
    if (!EnsureHas(json, "GltfNormalTextureInfo", "index")) return false;

    CheckGltfSupport(json, "GltfNormalTextureInfo", "scale");
    CheckGltfSupport(json, "GltfNormalTextureInfo", "extensions");
    CheckGltfSupport(json, "GltfNormalTextureInfo", "extras");

    textureInfo->index = json["index"].GetInt();
    textureInfo->texcoord = json.HasMember("texCoord") ? json["texCoord"].GetInt() : -1;

    return true;
}

static GltfPbrMetallicRoughness GltfPbrMetallicRoughness_Default()
{
    GltfPbrMetallicRoughness pbr;
    pbr.baseColorFactor = GltfVec4(1.0, 1.0, 1.0, 1.0);
    pbr.hasBaseColorTexture = false;

    return pbr;
}

static GltfPbrMetallicRoughness GltfPbrMetallicRoughness_Parse(const rapidjson::Value& json)
{
    // Must default this if parsing fails as per the spec.
    GltfPbrMetallicRoughness pbr = GltfPbrMetallicRoughness_Default();

    CheckGltfSupport(json, "GltfPbrMetallicRoughness", "metallicFactors");
    CheckGltfSupport(json, "GltfPbrMetallicRoughness", "roughnessFactor");
    CheckGltfSupport(json, "GltfPbrMetallicRoughness", "metallicRoughnessTexture");
    CheckGltfSupport(json, "GltfPbrMetallicRoughness", "extensions");
    CheckGltfSupport(json, "GltfPbrMetallicRoughness", "extras");

    pbr.baseColorFactor = Gltf_JsonGet(json, "baseColorFactor", GltfVec4(1.0, 1.0, 1.0, 1.0));
    pbr.hasBaseColorTexture = json.HasMember("baseColorTexture");
    if (pbr.hasBaseColorTexture) 
        pbr.hasBaseColorTexture = GltfTextureInfo_Parse(json["baseColorTexture"], &pbr.baseColorTexture);

    return pbr;
}

static bool Gltf_Parse(const rapidjson::Value& json, GltfMaterial* material)
{
    CheckGltfSupport(json, "GltfMaterial", "occlusionTexture");
    CheckGltfSupport(json, "GltfMaterial", "emissiveTexture");
    CheckGltfSupport(json, "GltfMaterial", "emissiveFactor");
    CheckGltfSupport(json, "GltfMaterial", "alphaMode");
    CheckGltfSupport(json, "GltfMaterial", "alphaCutoff");
    CheckGltfSupport(json, "GltfMaterial", "doubleSided");
    CheckGltfSupport(json, "GltfMaterial", "extensions");
    CheckGltfSupport(json, "GltfMaterial", "extras");

    material->name = json.HasMember("name") ? json["name"].GetString() : "";    
    material->pbr = json.HasMember("pbrMetallicRoughness") ? GltfPbrMetallicRoughness_Parse(json["pbrMetallicRoughness"]) : GltfPbrMetallicRoughness_Default();
    material->hasNormalTexture = json.HasMember("normalTexture");
    if (material->hasNormalTexture)
        material->hasNormalTexture = GltfNormalTextureInfo_Parse(json["normalTexture"], &material->normalTexture);

    return true;
}

static bool Gltf_Parse(const rapidjson::Value& json, GltfTexture* texture)
{
    CheckGltfSupport(json, "GltfTexture", "extensions");
    CheckGltfSupport(json, "GltfTexture", "extras");

    texture->name = json.HasMember("name") ? json["name"].GetString() : "";
    texture->sampler = json.HasMember("sampler") ? json["sampler"].GetInt() : -1;
    texture->source = json.HasMember("source") ? json["source"].GetInt() : -1;
    
    return true;
}

static bool Gltf_Parse(const rapidjson::Value& json, GltfSampler* sampler)
{
    CheckGltfSupport(json, "GltfSampler", "extensions");
    CheckGltfSupport(json, "GltfSampler", "extras");

    sampler->name = json.HasMember("name") ? json["name"].GetString() : "";
    sampler->magFilter = json.HasMember("magFilter") ? json["magFilter"].GetInt() : -1;
    sampler->minFilter = json.HasMember("minFilter") ? json["minFilter"].GetInt() : -1;
    sampler->wrapS = json.HasMember("wrapS") ? json["wrapS"].GetInt() : 10497;
    sampler->wrapT = json.HasMember("wrapT") ? json["wrapT"].GetInt() : 10497;

    return true;
}

static bool Gltf_Parse(const rapidjson::Value& json, GltfImage* image)
{
    CheckGltfSupport(json, "GltfImage", "extensions");
    CheckGltfSupport(json, "GltfImage", "extras");

    image->name = json.HasMember("name") ? json["name"].GetString() : "";
    image->uri = json.HasMember("uri") ? json["uri"].GetString() : "";
    image->mimeType = json.HasMember("mimeType") ? json["mimeType"].GetString() : "";
    image->bufferView = json.HasMember("bufferView") ? json["bufferView"].GetInt() : -1;

    return true;
}

static bool Gltf_Parse(const rapidjson::Value& json, GltfAccessor* accessor)
{
    if (!EnsureHas(json, "GltfAccessor", "componentType")) return false;
    if (!EnsureHas(json, "GltfAccessor", "count")) return false;
    if (!EnsureHas(json, "GltfAccessor", "type")) return false;

    CheckGltfSupport(json, "GltfAccessor", "sparse");
    CheckGltfSupport(json, "GltfAccessor", "extensions");
    CheckGltfSupport(json, "GltfAccessor", "extras");

    accessor->componentType = (GltfComponentType)json["componentType"].GetInt();
    accessor->count = json["count"].GetInt();

    const std::string elemType = json["type"].GetString();

    if      (elemType == "SCALAR") accessor->type = GltfElementType::SCALAR;
    else if (elemType == "VEC2") accessor->type = GltfElementType::VEC2;
    else if (elemType == "VEC3") accessor->type = GltfElementType::VEC3;
    else if (elemType == "VEC4") accessor->type = GltfElementType::VEC4;
    else if (elemType == "MAT2") accessor->type = GltfElementType::MAT2;
    else if (elemType == "MAT3") accessor->type = GltfElementType::MAT3;
    else if (elemType == "MAT4") accessor->type = GltfElementType::MAT4;

    accessor->name = json.HasMember("name") ? json["name"].GetString() : "";
    accessor->bufferView = json.HasMember("bufferView") ? json["bufferView"].GetInt() : -1;
    accessor->byteOffset = json.HasMember("byteOffset") ? json["byteOffset"].GetInt() : 0;
    accessor->normalized = json.HasMember("normalized") ? json["normalized"].GetBool() : false;
    
    if(json.HasMember("max"))
    {
        const rapidjson::Value& maxValue = json["max"];
        const rapidjson::SizeType maxCount = maxValue.Size();
        for (rapidjson::SizeType i = 0; i < maxCount; i++)
            accessor->max[i] = maxValue[i].GetDouble();
    }

    if (json.HasMember("min"))
    {
        const rapidjson::Value& minValue = json["min"];
        const rapidjson::SizeType minCount = minValue.Size();
        for (rapidjson::SizeType i = 0; i < minCount; i++)
            accessor->min[i] = minValue[i].GetDouble();
    }    

    return true;
}

static bool Gltf_Parse(const rapidjson::Value& json, GltfBufferView* bufferView)
{
    if (!EnsureHas(json, "GltfBufferView", "buffer")) return false;
    if (!EnsureHas(json, "GltfBufferView", "byteLength")) return false;

    CheckGltfSupport(json, "GltfBufferView", "extensions");
    CheckGltfSupport(json, "GltfBufferView", "extras");

    bufferView->buffer = json["buffer"].GetInt();
    bufferView->byteLength = json["byteLength"].GetInt();


    bufferView->name = json.HasMember("name") ? json["name"].GetString() : "";    
    bufferView->byteOffset = json.HasMember("byteOffset") ? json["byteOffset"].GetInt() : 0;
    bufferView->byteStride = json.HasMember("byteStride") ? json["byteStride"].GetInt() : -1;
    bufferView->target = json.HasMember("target") ? json["target"].GetInt() : -1;

    return true;
}

static bool Gltf_Parse(const rapidjson::Value& json, GltfBuffer* buffer)
{
    if(!EnsureHas(json, "GltfBuffer", "byteLength")) return false;

    CheckGltfSupport(json, "GltfBuffer", "extensions");
    CheckGltfSupport(json, "GltfBuffer", "extras");

    buffer->byteLength = json["byteLength"].GetInt();

    buffer->name = json.HasMember("name") ? json["name"].GetString() : "";
    buffer->uri = json.HasMember("uri") ? json["uri"].GetString() : "";

    return true;
}

template<typename GltfArray>
static bool GltfArray_Parse(const rapidjson::Value& json, GltfArray* arr)
{
    arr->resize(json.Size());
    for (rapidjson::SizeType i = 0; i < json.Size(); i++)
        if (!Gltf_Parse(json[i], &((*arr)[i]))) return false;

    return true;
}

static bool Gltf_Parse(const rapidjson::Value& json, Gltf* gltf)
{
    if (!EnsureHas(json, "Gltf", "asset")) return false;

    CheckGltfSupport(json, "Gltf", "animations");
    CheckGltfSupport(json, "Gltf", "cameras");
    CheckGltfSupport(json, "Gltf", "skins");
    CheckGltfSupport(json, "Gltf", "extensions");
    CheckGltfSupport(json, "Gltf", "extras");

    Gltf_Parse(json["asset"], &gltf->asset);

    auto ParseExtensionArray = [&](const char* key, std::vector<std::string>* arr)
    {
        if (!json.HasMember(key))
            return;

        const rapidjson::Value& val = json[key];
        const rapidjson::SizeType count = val.Size();

        arr->resize(count);

        for (rapidjson::SizeType i = 0; i < count; i++)
            (*arr)[i] = val[i].GetString();
    };

    ParseExtensionArray("extensionsUsed", &gltf->extensionsUsed);
    ParseExtensionArray("extensionsUsed", &gltf->extensionsRequired);

    bool succeeded = true;

    if (json.HasMember("accessors"))    succeeded &= GltfArray_Parse(json["accessors"],     &gltf->accessors);
    if (json.HasMember("buffers"))      succeeded &= GltfArray_Parse(json["buffers"],       &gltf->buffers);
    if (json.HasMember("bufferViews"))  succeeded &= GltfArray_Parse(json["bufferViews"],   &gltf->bufferViews);
    if (json.HasMember("images"))       succeeded &= GltfArray_Parse(json["images"],        &gltf->images);
    if (json.HasMember("materials"))    succeeded &= GltfArray_Parse(json["materials"],     &gltf->materials);
    if (json.HasMember("meshes"))       succeeded &= GltfArray_Parse(json["meshes"],        &gltf->meshes);
    if (json.HasMember("nodes"))        succeeded &= GltfArray_Parse(json["nodes"],         &gltf->nodes);
    if (json.HasMember("samplers"))     succeeded &= GltfArray_Parse(json["samplers"],      &gltf->samplers);
    if (json.HasMember("scenes"))       succeeded &= GltfArray_Parse(json["scenes"],        &gltf->scenes);
    if (json.HasMember("textures"))     succeeded &= GltfArray_Parse(json["textures"],      &gltf->textures);

    return succeeded;
}

bool GltfLoader_Load(const char* path, Gltf* loadedGltf)
{
    if (!loadedGltf)
        return false;

    *loadedGltf = {};

    std::vector<uint8_t> fileBuf = LoadBinaryFile(path);

    if (!ENSUREMSG(!fileBuf.empty(), "Failed to load Gltf file: %s", path))
        return false;

    bool parseSuccess = false;
    do
    {
        if (!ENSUREMSG(fileBuf.size() >= 12, "Gltf file does not have the correct header size: %s", path))
            break;

        GltfHdr* hdr = (GltfHdr*)fileBuf.data();

        if (!ENSUREMSG(hdr->magic == GltfMagic, "Gltf file does not have the correct file type: %s", path))
            break;

#if GLTF_LOG_ENABLED(GLTF_LOG_LEVEL_VERBOSE)
        LOGINFO("Gltf: Loading %s", path);
        LOGINFO("Gltf: Version %d", hdr->version);
        LOGINFO("Gltf: Length %d", hdr->length);
#endif

        if (!ENSUREMSG(hdr->version == 2, "Gltf file version must be 2, %d is not supported", hdr->version))
            break;

        uint8_t* chunkStart = fileBuf.data() + sizeof(GltfHdr);

        GltfChunk* jsonChunk = (GltfChunk*)chunkStart;

        if (!ENSUREMSG(jsonChunk->type == GltfJsonChunk, "Gltf first chunk must be JSON type: %s", path))
            break;

        // Load JSON
        {
            const char* jsonStr = (const char*)chunkStart + sizeof(jsonChunk);
            rapidjson::Document json;
            json.Parse(jsonStr, jsonChunk->length);

            if (!ENSUREMSG(!json.HasParseError(), "Gltf: failed to parse json chunk with code %d", json.GetParseError()))
                break;

            if (!ENSUREMSG(Gltf_Parse(json, loadedGltf), "Gltf: Failed to parse"))
                break;

#if GLTF_LOG_ENABLED(GLTF_LOG_LEVEL_VERBOSE)
            LOGINFO("Gltf: %d accessors", loadedGltf->accessors.size());
            LOGINFO("Gltf: %d buffers", loadedGltf->buffers.size());
            LOGINFO("Gltf: %d bufferViews", loadedGltf->bufferViews.size());
            LOGINFO("Gltf: %d images", loadedGltf->images.size());
            LOGINFO("Gltf: %d materials", loadedGltf->materials.size());
            LOGINFO("Gltf: %d meshes", loadedGltf->meshes.size());
            LOGINFO("Gltf: %d nodes", loadedGltf->nodes.size());
            LOGINFO("Gltf: %d samplers", loadedGltf->samplers.size());
            LOGINFO("Gltf: %d scenes", loadedGltf->scenes.size());
            LOGINFO("Gltf: %d textures", loadedGltf->textures.size());
#endif
        }

        // Load Binary
        {
            chunkStart = chunkStart + sizeof(GltfChunk) + jsonChunk->length;
            GltfChunk* binChunk = (GltfChunk*)chunkStart;

            uint8_t* binData = chunkStart + sizeof(GltfChunk);

            loadedGltf->data = std::make_unique<uint8_t[]>(binChunk->length);
            memcpy(loadedGltf->data.get(), binData, binChunk->length);
        }

        parseSuccess = true;
    } while(false);

    return parseSuccess;
}

size_t GltfLoader_SizeOfComponent(GltfComponentType ct)
{
    switch (ct)
    {
    case GltfComponentType::BYTE:
    case GltfComponentType::UNSIGNED_BYTE:
        return 1;
    case GltfComponentType::SHORT:
    case GltfComponentType::UNSIGNED_SHORT:
        return 2;
    case GltfComponentType::UNSIGNED_INT:
    case GltfComponentType::FLOAT:
        return 4;
    };

    return 0;
}

size_t GltfLoader_ComponentCount(GltfElementType et)
{
    switch (et)
    {
    case GltfElementType::SCALAR:
        return 1;
    case GltfElementType::VEC2:
        return 2;
    case GltfElementType::VEC3:
        return 3;
    case GltfElementType::VEC4:
    case GltfElementType::MAT2:
        return 4;
    case GltfElementType::MAT3:
        return 9;
    case GltfElementType::MAT4:
        return 16;
    }

    return 0;
}
