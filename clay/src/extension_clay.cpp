#if !defined(DM_HEADLESS)

#define CLAY_IMPLEMENTATION
#include "clay.h"

#include <dmsdk/sdk.h>
#include <dmsdk/dlib/hashtable.h>
#include <dmsdk/dlib/math.h>
#include <dmsdk/dlib/memory.h>
#include <dmsdk/dlib/profile.h>
#include <dmsdk/gamesys/render_constants.h>
#include <dmsdk/gamesys/resources/res_font.h>
#include <dmsdk/gui/gui.h>
#include <float.h>

#if defined(DEFOLD_CLAY_EXTENSION)
// This isn't neat. But a lot of function we require in dmGui aren't available to Defold extensions,
// so we must declare those functions and structs here.

#if defined(_WIN32) && defined(GetTextMetrics)
#undef GetTextMetrics
#endif

namespace dmGui
{
    struct TextMetrics
    {
        /// Default constructor, initializes everything to 0
        TextMetrics();

        /// Total string width
        float m_Width;
        /// Total string height
        float m_Height;
        /// Max ascent of font
        float m_MaxAscent;
        /// Max descent of font, positive value
        float m_MaxDescent;
    };

    enum Pivot
    {
        PIVOT_CENTER = 0,
        PIVOT_N = 1,
        PIVOT_NE = 2,
        PIVOT_E = 3,
        PIVOT_SE = 4,
        PIVOT_S = 5,
        PIVOT_SW = 6,
        PIVOT_W = 7,
        PIVOT_NW = 8,
    };

    enum SizeMode
    {
        SIZE_MODE_MANUAL = 0,
        SIZE_MODE_AUTO = 1,
    };

    enum ClippingMode
    {
        CLIPPING_MODE_NONE = 0,
        CLIPPING_MODE_STENCIL = 2,
    };

    typedef struct TextLayout* HTextLayout;

    struct TextLayout
    {
        HTextLayout m_Handle;
        uint64_t    m_Key;
    };

    typedef void (*AnimationComplete)(HScene scene,
                                      HNode  node,
                                      bool   finished,
                                      void*  userdata1,
                                      void*  userdata2);

    dmVMath::Point3  GetNodeSize(HScene scene, HNode node);
    dmVMath::Matrix4 GetNodeWorldTransform(HScene scene, HNode node);

    Result           GetTextMetrics(HScene scene, const char* text, dmhash_t font_id, float width, bool line_break, float leading, float tracking, TextMetrics* metrics);
    void             SetNodePivot(HScene scene, HNode node, Pivot pivot);
    void             SetNodeInheritAlpha(HScene scene, HNode node, bool inherit_alpha);
    void             SetNodeSizeMode(HScene scene, HNode node, SizeMode size_mode);
    Pivot            GetNodePivot(HScene scene, HNode node);
    Result           PlayNodeFlipbookAnim(HScene scene, HNode node, dmhash_t anim, float offset, float playback_rate, AnimationComplete anim_complete_callback = 0x0, void* callback_userdata1 = 0x0, void* callback_userdata2 = 0x0);
    int32_t          GetNodeAnimationFrameCount(HScene scene, HNode node);
    void             CancelNodeFlipbookAnim(HScene scene, HNode node, bool keep_anim_hash);
    void             MoveNodeAbove(HScene scene, HNode node, HNode reference);
    void             MoveNodeBelow(HScene scene, HNode node, HNode reference);
    Result           SetNodeFont(HScene scene, HNode node, dmhash_t font_id);
    void             SetNodeText(HScene scene, HNode node, const char* text);
    void             SetNodeLineBreak(HScene scene, HNode node, bool line_break);
    void             SetNodeTextLeading(HScene scene, HNode node, float leading);
    void             SetNodeTextTracking(HScene scene, HNode node, float tracking);
    void             SetNodeClippingMode(HScene scene, HNode node, ClippingMode mode);
    void             SetNodeClippingVisible(HScene scene, HNode node, bool visible);
    void             SetNodeClippingInverted(HScene scene, HNode node, bool inverted);
    Result           SetNodeLayer(HScene scene, HNode node, dmhash_t layer_id);
    const void*      GetNodeRenderConstants(HScene scene, HNode node);
    void             SetNodeRenderConstants(HScene scene, HNode node, void* render_constants);
    const float*     GetNodeFlipbookAnimUV(HScene scene, HNode node);
    void             GetNodeFlipbookAnimUVFlip(HScene scene, HNode node, bool& flip_horizontal, bool& flip_vertical);
    void*            GetFont(HScene scene, dmhash_t font_hash);
} // namespace dmGui
#else
#include <gui/gui.h>
#endif // DEFOLD_CLAY_EXTENSION

#define MODULE_NAME "clay"
#define ID_META "clay.id_descriptor"
#define SIZING_META "clay.sizing_descriptor"
#define TEXT_META "clay.text_descriptor"
#define SURFACE_META "clay.surface"

static const int32_t DCLAY_DEFAULT_MAX_ELEMENT_COUNT = 8192;

#if defined(DM_DEBUG)
DM_PROPERTY_GROUP(rmtp_Clay, "Clay", 0);
DM_PROPERTY_U32(rmtp_ClaySurfaces, 0, PROFILE_PROPERTY_NONE, "# live Clay GUI surfaces", &rmtp_Clay);
DM_PROPERTY_U32(rmtp_ClayNodes, 0, PROFILE_PROPERTY_NONE, "# retained Clay GUI nodes", &rmtp_Clay);
DM_PROPERTY_U32(rmtp_ClayMemory, 0, PROFILE_PROPERTY_NONE, "Clay core and binding reserved memory in bytes", &rmtp_Clay);
DM_PROPERTY_U32(rmtp_ClayLayouts, 0, PROFILE_PROPERTY_FRAME_RESET, "# completed Clay layouts / frame", &rmtp_Clay);
DM_PROPERTY_U32(rmtp_ClayElements, 0, PROFILE_PROPERTY_FRAME_RESET, "# emitted Clay elements / frame", &rmtp_Clay);
DM_PROPERTY_U32(rmtp_ClayCommands, 0, PROFILE_PROPERTY_FRAME_RESET, "# Clay render commands / frame", &rmtp_Clay);
DM_PROPERTY_U32(rmtp_ClayNodesAdded, 0, PROFILE_PROPERTY_FRAME_RESET, "# retained GUI nodes added / frame", &rmtp_Clay);
DM_PROPERTY_U32(rmtp_ClayNodesRemoved, 0, PROFILE_PROPERTY_FRAME_RESET, "# retained GUI nodes removed / frame", &rmtp_Clay);
DM_PROPERTY_U32(rmtp_ClayNodesReused, 0, PROFILE_PROPERTY_FRAME_RESET, "# retained GUI nodes reused / frame", &rmtp_Clay);
DM_PROPERTY_U32(rmtp_ClayTextMeasurements, 0, PROFILE_PROPERTY_FRAME_RESET, "# Clay text measurements / frame", &rmtp_Clay);
#endif // DM_DEBUG

#define CONCAT_(prefix, suffix) prefix##suffix
#define CONCAT(prefix, suffix) CONCAT_(prefix, suffix)
#define UNIQUE_VAR(name) CONCAT(name, __LINE__)
#define PAD(size) char UNIQUE_VAR(_padding_)[size]

#if defined(_WIN32) && defined(_MSC_VER)
#define STRUCT_ALIGN(a) __declspec(align(a))
#else
#define STRUCT_ALIGN(a) __attribute__((aligned(a)))
#endif

enum dclay_id_type_t
{
    DCLAY_ID_GLOBAL,
    DCLAY_ID_GLOBAL_INDEXED,
    DCLAY_ID_LOCAL,
    DCLAY_ID_LOCAL_INDEXED,
};

// Represent an element id.
struct dclay_id_t
{
    dclay_id_type_t type;
    uint32_t        index;
    Clay_String     string;
};

struct dclay_font_t
{
    dmhash_t alias;
    float    base_size;
    bool     valid;
    PAD(3);
};

struct dclay_image_t
{
    dmhash_t texture;
    dmhash_t animation;
};

// Defold user data associated with certain clay render commands.
struct dclay_user_data_t
{
    dmhash_t         layer;
    dmVMath::Vector4 slice9;
};

enum dclay_gui_node_type_t
{
    DCLAY_GUI_NODE_RECTANGLE,
    DCLAY_GUI_NODE_TEXT,
    DCLAY_GUI_NODE_IMAGE,
    DCLAY_GUI_NODE_BORDER,
    DCLAY_GUI_NODE_CLIP,
};

STRUCT_ALIGN(8)
struct dclay_gui_node_t
{
    dmGui::HNode          node;
    uint32_t              generation;
    dclay_gui_node_type_t type;
    bool                  rounded_rect_constants_set;
    bool                  shape_uv_transform_set;
    PAD(2);
    dmhash_t         texture;
    dmhash_t         animation;
    dmhash_t         layer;
    dmVMath::Vector4 slice9;
    dmVMath::Vector4 corner_radii;
    dmVMath::Vector4 shape_uv_transform_x;
    dmVMath::Vector4 shape_uv_transform_y;
};

STRUCT_ALIGN(8)
struct dclay_surface_t
{
    bool valid;
    bool layout_open;
    PAD(2);
    void*         clay_memory;
    uint32_t      clay_memory_size;
    uint32_t      profiled_memory_bytes;
    Clay_Context* clay_context;
    dmGui::HScene gui_scene;
    dmGui::HNode  root_node;
    PAD(4);
    Clay_Vector2                    root_screen_scale;
    dmArray<int>                    root_refs;
    dmArray<dclay_font_t>           fonts;
    dmArray<dclay_image_t>          images;
    dmArray<dclay_user_data_t>      user_data;
    dmHashTable64<dclay_gui_node_t> gui_nodes;
    dmArray<char>                   text_scratch;
    uint32_t                        generation;
    PAD(4);
};

static dclay_surface_t* g_ActiveSurface = 0;
static bool             g_PixelPerfect = 0;

#if defined(DM_DEBUG)
static uint32_t g_SurfaceCount = 0;
static uint32_t g_NodeCount = 0;
static uint32_t g_MemoryBytes = 0;

template <typename T>
static uint64_t dclay_ArrayReservedBytes(const dmArray<T>& array)
{
    return (uint64_t)array.Capacity() * sizeof(T);
}

template <typename T>
static uint64_t dclay_HashTableReservedBytes(const dmHashTable64<T>& table)
{
    uint32_t capacity = table.Capacity();
    if (capacity == 0)
    {
        return 0;
    }

    uint32_t table_size = (capacity * 2) / 3;
    if (table_size == 0)
    {
        table_size = 1;
    }

    return dmHashTable64<T>::GetMemorySize(table_size, capacity);
}

static uint32_t dclay_CalculateReservedMemory(const dclay_surface_t* surface)
{
    return surface->clay_memory_size +
    dclay_ArrayReservedBytes(surface->root_refs) +
    dclay_ArrayReservedBytes(surface->fonts) +
    dclay_ArrayReservedBytes(surface->images) +
    dclay_ArrayReservedBytes(surface->user_data) +
    dclay_HashTableReservedBytes(surface->gui_nodes) +
    dclay_ArrayReservedBytes(surface->text_scratch);
}

static void dclay_UpdateMemoryProfile(dclay_surface_t* surface)
{
    uint32_t memory = dclay_CalculateReservedMemory(surface);

    if (memory >= surface->profiled_memory_bytes)
    {
        g_MemoryBytes += memory - surface->profiled_memory_bytes;
    }
    else
    {
        uint32_t released = surface->profiled_memory_bytes - memory;
        g_MemoryBytes = released <= g_MemoryBytes ? g_MemoryBytes - released : 0;
    }

    surface->profiled_memory_bytes = memory;
    DM_PROPERTY_SET_U32(rmtp_ClayMemory, g_MemoryBytes);
}

static void dclay_RemoveMemoryProfile(dclay_surface_t* surface)
{
    g_MemoryBytes = surface->profiled_memory_bytes <= g_MemoryBytes ? g_MemoryBytes - surface->profiled_memory_bytes : 0;
    surface->profiled_memory_bytes = 0;
    DM_PROPERTY_SET_U32(rmtp_ClayMemory, g_MemoryBytes);
}

static void dclay_RecordNodesAdded(uint32_t count)
{
    g_NodeCount += count;
    DM_PROPERTY_SET_U32(rmtp_ClayNodes, g_NodeCount);
    DM_PROPERTY_ADD_U32(rmtp_ClayNodesAdded, count);
}

static void dclay_RecordNodesRemoved(uint32_t count)
{
    g_NodeCount = count < g_NodeCount ? g_NodeCount - count : 0;
    DM_PROPERTY_SET_U32(rmtp_ClayNodes, g_NodeCount);
    DM_PROPERTY_ADD_U32(rmtp_ClayNodesRemoved, count);
}
#endif // DM_DEBUG

static int dclay_AbsIndex(lua_State* L, int index)
{
    return index > 0 || index <= LUA_REGISTRYINDEX ? index : lua_gettop(L) + index + 1;
}

static void dclay_ErrorHandler(Clay_ErrorData error_data)
{
    char   message[1024];
    size_t len = (size_t)error_data.errorText.length;
    if (len >= sizeof(message))
    {
        len = sizeof(message) - 1;
    }

    memcpy(message, error_data.errorText.chars, len);
    message[len] = 0;

    dmLogError("%s", message);
}

static dclay_surface_t* dclay_CheckSurface(lua_State* L, int index)
{
    dclay_surface_t** surface = (dclay_surface_t**)luaL_checkudata(L, index, SURFACE_META);
    if (!surface || !*surface || !(*surface)->valid)
    {
        luaL_error(L, "Clay surface has been destroyed");
    }

    return *surface;
}

static void dclay_SelectSurface(dclay_surface_t* surface)
{
    g_ActiveSurface = surface;
    Clay_SetCurrentContext(surface->clay_context);
}

static Clay_Dimensions dclay_GetRootDimensions(dclay_surface_t* surface, Clay_Vector2* out_screen_scale)
{
    dmVMath::Point3  size = dmGui::GetNodeSize(surface->gui_scene, surface->root_node);
    dmVMath::Matrix4 world = dmGui::GetNodeWorldTransform(surface->gui_scene, surface->root_node);
    dmVMath::Vector4 x_axis = world.getCol0();
    dmVMath::Vector4 y_axis = world.getCol1();
    Clay_Vector2     screen_scale = {
        sqrtf(x_axis.getX() * x_axis.getX() + x_axis.getY() * x_axis.getY()),
        sqrtf(y_axis.getX() * y_axis.getX() + y_axis.getY() * y_axis.getY())
    };

    if (screen_scale.x <= FLT_EPSILON)
    {
        screen_scale.x = 1.0f;
    }

    if (screen_scale.y <= FLT_EPSILON)
    {
        screen_scale.y = 1.0f;
    }

    if (out_screen_scale)
    {
        *out_screen_scale = screen_scale;
    }

    return { size.getX() * screen_scale.x, size.getY() * screen_scale.y };
}

static void dclay_ClearRoots(lua_State* L, dclay_surface_t* surface)
{
    for (uint32_t i = 0; i < surface->root_refs.Size(); ++i)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, surface->root_refs[i]);
    }

    surface->root_refs.SetSize(0);
}

static bool dclay_HasMetatable(lua_State* L, int index, const char* name)
{
    if (!lua_getmetatable(L, index))
    {
        return false;
    }

    luaL_getmetatable(L, name);
    bool equal = lua_rawequal(L, -1, -2) != 0;
    lua_pop(L, 2);

    return equal;
}

static float dclay_GetOptionalNumberField(lua_State* L, int table_index, const char* name, float default_value)
{
    table_index = dclay_AbsIndex(L, table_index);

    lua_getfield(L, table_index, name);
    float value = lua_isnil(L, -1) ? default_value : (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    return value;
}

static bool dclay_GetOptionalBoolField(lua_State* L, int table_index, const char* name, bool default_value)
{
    table_index = dclay_AbsIndex(L, table_index);

    lua_getfield(L, table_index, name);
    bool value = lua_isnil(L, -1) ? default_value : lua_toboolean(L, -1) != 0;
    lua_pop(L, 1);

    return value;
}

static uint16_t dclay_ToU16(lua_State* L, int index, const char* field)
{
    lua_Integer value = luaL_checkinteger(L, index);
    if (value < 0 || value > 65535)
    {
        luaL_error(L, "clay field '%s' must be between 0 and 65535", field);
    }

    return (uint16_t)value;
}

static uint16_t dclay_GetOptionalU16Field(lua_State* L, int table_index, const char* name, uint16_t default_value)
{
    table_index = dclay_AbsIndex(L, table_index);

    lua_getfield(L, table_index, name);
    uint16_t value = lua_isnil(L, -1) ? default_value : dclay_ToU16(L, -1, name);
    lua_pop(L, 1);

    return value;
}

static int32_t dclay_GetOptionalIntegerField(lua_State* L, int table_index, const char* name, int32_t default_value, int32_t minimum, int32_t maximum)
{
    table_index = dclay_AbsIndex(L, table_index);

    lua_getfield(L, table_index, name);
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        return default_value;
    }

    lua_Integer value = luaL_checkinteger(L, -1);
    if (value < minimum || value > maximum)
    {
        luaL_error(L, "clay field '%s' must be between %d and %d", name, minimum, maximum);
    }
    lua_pop(L, 1);

    return (int32_t)value;
}

static Clay_String dclay_CheckString(lua_State* L, int index)
{
    size_t      length = 0;
    const char* text = luaL_checklstring(L, index, &length);

    Clay_String result = {};
    result.length = (int32_t)length;
    result.chars = text;

    return result;
}

static Clay_ElementId dclay_GetElementId(lua_State* L, int index)
{
    if (lua_isstring(L, index))
    {
        return Clay__HashString(dclay_CheckString(L, index), 0);
    }

    if (dmScript::IsHash(L, index))
    {
        uint64_t       hash = dmScript::CheckHash(L, index);
        Clay_ElementId result = {};
        result.id = (uint32_t)hash ^ (uint32_t)(hash >> 32);
        return result;
    }

    if (!dclay_HasMetatable(L, index, ID_META))
    {
        luaL_error(L, "clay element field 'id' must be nil, a string, a hash, or a value returned by a clay id helper");
    }

    dclay_id_t* desc = (dclay_id_t*)lua_touserdata(L, index);
    uint32_t    seed = (desc->type == DCLAY_ID_LOCAL || desc->type == DCLAY_ID_LOCAL_INDEXED) ? Clay_GetOpenElementId() : 0;

    if (desc->type == DCLAY_ID_GLOBAL_INDEXED || desc->type == DCLAY_ID_LOCAL_INDEXED)
    {
        return Clay__HashStringWithOffset(desc->string, desc->index, seed);
    }

    return Clay__HashString(desc->string, seed);
}

static Clay_ElementId dclay_GetQueryableElementId(lua_State* L, int index)
{
    if (dclay_HasMetatable(L, index, ID_META))
    {
        dclay_id_t* desc = (dclay_id_t*)lua_touserdata(L, index);
        if (desc->type == DCLAY_ID_LOCAL || desc->type == DCLAY_ID_LOCAL_INDEXED)
        {
            luaL_error(L, "local Clay ids cannot be queried outside their parent traversal; use a global id");
        }
    }

    return dclay_GetElementId(L, index);
}

static void dclay_ValidateElementId(lua_State* L, int index)
{
    if (lua_isnil(L, index) || lua_isstring(L, index) || dmScript::IsHash(L, index) || dclay_HasMetatable(L, index, ID_META))
    {
        return;
    }

    luaL_error(L, "clay element field 'id' must be nil, a string, a hash, or a value returned by a clay id helper");
}

static void dclay_ParsePadding(lua_State* L, int index, Clay_Padding* padding)
{
    index = dclay_AbsIndex(L, index);
    if (lua_isnumber(L, index))
    {
        uint16_t value = dclay_ToU16(L, index, "padding");
        padding->left = padding->right = padding->top = padding->bottom = value;
        return;
    }

    luaL_checktype(L, index, LUA_TTABLE);

    padding->left = (uint16_t)dclay_GetOptionalNumberField(L, index, "left", 0);
    padding->right = (uint16_t)dclay_GetOptionalNumberField(L, index, "right", 0);
    padding->top = (uint16_t)dclay_GetOptionalNumberField(L, index, "top", 0);
    padding->bottom = (uint16_t)dclay_GetOptionalNumberField(L, index, "bottom", 0);
}

static void dclay_ParseCornerRadius(lua_State* L, int index, Clay_CornerRadius* radius)
{
    index = dclay_AbsIndex(L, index);
    if (lua_isnumber(L, index))
    {
        float value = (float)lua_tonumber(L, index);
        radius->topLeft = radius->topRight = radius->bottomLeft = radius->bottomRight = value;
        return;
    }

    luaL_checktype(L, index, LUA_TTABLE);
    float values[4] = {};

    for (int i = 0; i < 4; ++i)
    {
        lua_rawgeti(L, index, i + 1);
        values[i] = (float)luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }

    radius->topLeft = values[0];
    radius->topRight = values[1];
    radius->bottomRight = values[2];
    radius->bottomLeft = values[3];
}

static void dclay_ParseColor(lua_State* L, int index, Clay_Color* color)
{
    index = dclay_AbsIndex(L, index);

    dmVMath::Vector4* v4 = dmScript::ToVector4(L, index);
    if (v4)
    {
        color->r = v4->getX() * 255.0f;
        color->g = v4->getY() * 255.0f;
        color->b = v4->getZ() * 255.0f;
        color->a = v4->getW() * 255.0f;
        return;
    }

    dmVMath::Vector3* v3 = dmScript::ToVector3(L, index);
    if (v3)
    {
        color->r = v3->getX() * 255.0f;
        color->g = v3->getY() * 255.0f;
        color->b = v3->getZ() * 255.0f;
        color->a = 255.0f;
        return;
    }

    luaL_error(L, "clay color must be vmath.vector3 or vmath.vector4");
}

static dmhash_t dclay_CheckHashOrString(lua_State* L, int index, const char* field)
{
    if (lua_isstring(L, index))
    {
        return dmHashString64(lua_tostring(L, index));
    }

    if (dmScript::IsHash(L, index))
    {
        return dmScript::CheckHash(L, index);
    }

    return (dmhash_t)luaL_error(L, "clay field '%s' must be a string or Defold hash", field);
}

static dmVMath::Vector4 dclay_CheckVector4(lua_State* L, int index, const char* field)
{
    dmVMath::Vector4* value = dmScript::ToVector4(L, index);
    if (!value)
    {
        luaL_error(L, "clay field '%s' must be vmath.vector4", field);
    }

    return *value;
}

static bool dclay_IsZeroVector4(const dmVMath::Vector4& value)
{
    return value.getX() == 0.0f && value.getY() == 0.0f && value.getZ() == 0.0f && value.getW() == 0.0f;
}

static bool dclay_Vector4Equal(const dmVMath::Vector4& a, const dmVMath::Vector4& b)
{
    return a.getX() == b.getX() && a.getY() == b.getY() && a.getZ() == b.getZ() && a.getW() == b.getW();
}

static dclay_user_data_t dclay_ParseUserData(lua_State* L, int table_index)
{
    dclay_user_data_t user_data;
    user_data.layer = 0;
    user_data.slice9 = dmVMath::Vector4(0.0f);

    table_index = dclay_AbsIndex(L, table_index);

    lua_getfield(L, table_index, "layer");
    if (!lua_isnil(L, -1))
    {
        user_data.layer = dclay_CheckHashOrString(L, -1, "layer");
    }
    lua_pop(L, 1);

    lua_getfield(L, table_index, "slice9");
    if (!lua_isnil(L, -1))
    {
        user_data.slice9 = dclay_CheckVector4(L, -1, "slice9");
    }
    lua_pop(L, 1);

    return user_data;
}

static dclay_user_data_t* dclay_StoreUserData(lua_State* L, const dclay_user_data_t& value)
{
    if (value.layer == 0 && dclay_IsZeroVector4(value.slice9))
    {
        return 0;
    }

    if (g_ActiveSurface->user_data.Full())
    {
        luaL_error(L, "Clay layout userdata exhausted (%u entries)", g_ActiveSurface->user_data.Capacity());
    }

    uint32_t user_data_index = g_ActiveSurface->user_data.Size();
    g_ActiveSurface->user_data.SetSize(user_data_index + 1);
    dclay_user_data_t* user_data = &g_ActiveSurface->user_data[user_data_index];
    *user_data = value;

    return user_data;
}

static void dclay_ParseBorderWidth(lua_State* L, int index, Clay_BorderWidth* width)
{
    index = dclay_AbsIndex(L, index);

    if (lua_isnumber(L, index))
    {
        uint16_t value = dclay_ToU16(L, index, "border.width");
        width->left = width->right = width->top = width->bottom = value;

        return;
    }

    luaL_checktype(L, index, LUA_TTABLE);

    width->left = dclay_GetOptionalU16Field(L, index, "left", 0);
    width->right = dclay_GetOptionalU16Field(L, index, "right", 0);
    width->top = dclay_GetOptionalU16Field(L, index, "top", 0);
    width->bottom = dclay_GetOptionalU16Field(L, index, "bottom", 0);
    width->betweenChildren = dclay_GetOptionalU16Field(L, index, "between_children", 0);
}

static void dclay_ParseTransition(lua_State* L, int index, Clay_TransitionElementConfig* transition)
{
    index = dclay_AbsIndex(L, index);
    luaL_checktype(L, index, LUA_TTABLE);

    lua_getfield(L, index, "duration");
    if (lua_isnil(L, -1))
    {
        luaL_error(L, "clay transition requires a duration field");
    }

    transition->duration = (float)luaL_checknumber(L, -1);
    if (transition->duration < 0.0f)
    {
        luaL_error(L, "clay transition duration must be zero or greater");
    }
    lua_pop(L, 1);

    const int32_t valid_properties = CLAY_TRANSITION_PROPERTY_BOUNDING_BOX |
    CLAY_TRANSITION_PROPERTY_BACKGROUND_COLOR |
    CLAY_TRANSITION_PROPERTY_OVERLAY_COLOR |
    CLAY_TRANSITION_PROPERTY_BORDER;

    lua_getfield(L, index, "properties");
    if (lua_isnil(L, -1))
    {
        luaL_error(L, "clay transition requires a properties bitmask");
    }

    int32_t properties = (int32_t)luaL_checkinteger(L, -1);
    if (properties <= 0 || (properties & ~valid_properties) != 0)
    {
        luaL_error(L, "clay transition properties contains an unsupported flag");
    }
    transition->properties = (Clay_TransitionProperty)properties;
    lua_pop(L, 1);

    transition->interactionHandling = (Clay_TransitionInteractionHandlingType)dclay_GetOptionalIntegerField(L, index, "interaction_handling", CLAY_TRANSITION_DISABLE_INTERACTIONS_WHILE_TRANSITIONING_POSITION, CLAY_TRANSITION_DISABLE_INTERACTIONS_WHILE_TRANSITIONING_POSITION, CLAY_TRANSITION_ALLOW_INTERACTIONS_WHILE_TRANSITIONING_POSITION);
    transition->handler = Clay_EaseOut;

    // transition enter and exit functions aren't implemented yet.
    //  lua_getfield(L, index, "enter");
    //  if (!lua_isnil(L, -1))
    //  {
    //      luaL_error(L, "clay transition enter callbacks are not implemented yet");
    //  }
    //  lua_pop(L, 1);

    //  lua_getfield(L, index, "exit");
    //  if (!lua_isnil(L, -1))
    //  {
    //      luaL_error(L, "clay transition exit callbacks are not implemented yet");
    //  }
    //  lua_pop(L, 1);
}

static Clay_SizingAxis dclay_CheckSizingAxis(lua_State* L, int index)
{
    return *(Clay_SizingAxis*)luaL_checkudata(L, index, SIZING_META);
}

static void dclay_ParseLayout(lua_State* L, int index, Clay_LayoutConfig* layout)
{
    index = dclay_AbsIndex(L, index);

    lua_getfield(L, index, "padding");
    if (!lua_isnil(L, -1))
    {
        dclay_ParsePadding(L, -1, &layout->padding);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "sizing");
    if (!lua_isnil(L, -1))
    {
        luaL_checktype(L, -1, LUA_TTABLE);
        int sizing_index = dclay_AbsIndex(L, -1);

        lua_getfield(L, sizing_index, "width");
        if (!lua_isnil(L, -1))
        {
            layout->sizing.width = dclay_CheckSizingAxis(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, sizing_index, "height");
        if (!lua_isnil(L, -1))
        {
            layout->sizing.height = dclay_CheckSizingAxis(L, -1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    layout->childGap = (uint16_t)dclay_GetOptionalNumberField(L, index, "child_gap", layout->childGap);
    layout->layoutDirection = (Clay_LayoutDirection)(int)dclay_GetOptionalNumberField(L, index, "layout_direction", layout->layoutDirection);

    lua_getfield(L, index, "child_alignment");
    if (!lua_isnil(L, -1))
    {
        luaL_checktype(L, -1, LUA_TTABLE);
        layout->childAlignment.x = (Clay_LayoutAlignmentX)(int)dclay_GetOptionalNumberField(L, -1, "x", layout->childAlignment.x);
        layout->childAlignment.y = (Clay_LayoutAlignmentY)(int)dclay_GetOptionalNumberField(L, -1, "y", layout->childAlignment.y);
    }
    lua_pop(L, 1);
}

static void dclay_ParseFloating(lua_State* L, int index, Clay_FloatingElementConfig* floating)
{
    index = dclay_AbsIndex(L, index);
    luaL_checktype(L, index, LUA_TTABLE);

    lua_getfield(L, index, "offset");
    if (!lua_isnil(L, -1))
    {
        luaL_checktype(L, -1, LUA_TTABLE);
        floating->offset.x = dclay_GetOptionalNumberField(L, -1, "x", 0.0f);
        floating->offset.y = dclay_GetOptionalNumberField(L, -1, "y", 0.0f);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "expand");
    if (!lua_isnil(L, -1))
    {
        luaL_checktype(L, -1, LUA_TTABLE);
        floating->expand.width = dclay_GetOptionalNumberField(L, -1, "width", 0.0f);
        floating->expand.height = dclay_GetOptionalNumberField(L, -1, "height", 0.0f);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "parent_id");
    if (!lua_isnil(L, -1))
    {
        floating->parentId = dclay_GetQueryableElementId(L, -1).id;
    }
    lua_pop(L, 1);

    floating->zIndex = (int16_t)dclay_GetOptionalIntegerField(L, index, "z_index", 0, INT16_MIN, INT16_MAX);

    lua_getfield(L, index, "attach_points");
    if (!lua_isnil(L, -1))
    {
        luaL_checktype(L, -1, LUA_TTABLE);
        floating->attachPoints.element = (Clay_FloatingAttachPointType)dclay_GetOptionalIntegerField(L, -1, "element", CLAY_ATTACH_POINT_LEFT_TOP, CLAY_ATTACH_POINT_LEFT_TOP, CLAY_ATTACH_POINT_RIGHT_BOTTOM);
        floating->attachPoints.parent = (Clay_FloatingAttachPointType)dclay_GetOptionalIntegerField(L, -1, "parent", CLAY_ATTACH_POINT_LEFT_TOP, CLAY_ATTACH_POINT_LEFT_TOP, CLAY_ATTACH_POINT_RIGHT_BOTTOM);
    }
    lua_pop(L, 1);

    floating->pointerCaptureMode = (Clay_PointerCaptureMode)dclay_GetOptionalIntegerField(L, index, "pointer_capture_mode", CLAY_POINTER_CAPTURE_MODE_CAPTURE, CLAY_POINTER_CAPTURE_MODE_CAPTURE, CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH);
    floating->attachTo = (Clay_FloatingAttachToElement)dclay_GetOptionalIntegerField(L, index, "attach_to", CLAY_ATTACH_TO_NONE, CLAY_ATTACH_TO_NONE, CLAY_ATTACH_TO_ROOT);
    floating->clipTo = (Clay_FloatingClipToElement)dclay_GetOptionalIntegerField(L, index, "clip_to", CLAY_CLIP_TO_NONE, CLAY_CLIP_TO_NONE, CLAY_CLIP_TO_ATTACHED_PARENT);

    if (floating->attachTo == CLAY_ATTACH_TO_ELEMENT_WITH_ID && floating->parentId == 0)
    {
        luaL_error(L, "clay floating field 'parent_id' is required when attach_to is ATTACH_TO_ELEMENT_WITH_ID");
    }
}

static bool dclay_ParseElement(lua_State* L, int index, Clay_ElementDeclaration* declaration)
{
    bool use_scroll_offset = false;
    index = dclay_AbsIndex(L, index);

    lua_getfield(L, index, "layout");
    if (!lua_isnil(L, -1))
    {
        luaL_checktype(L, -1, LUA_TTABLE);
        dclay_ParseLayout(L, -1, &declaration->layout);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "background_color");
    if (!lua_isnil(L, -1))
    {
        dclay_ParseColor(L, -1, &declaration->backgroundColor);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "overlay_color");
    if (!lua_isnil(L, -1))
    {
        dclay_ParseColor(L, -1, &declaration->overlayColor);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "corner_radius");
    if (!lua_isnil(L, -1))
    {
        dclay_ParseCornerRadius(L, -1, &declaration->cornerRadius);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "floating");
    if (!lua_isnil(L, -1))
    {
        dclay_ParseFloating(L, -1, &declaration->floating);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "image");
    if (!lua_isnil(L, -1))
    {
        dclay_image_t image = {};

        if (lua_istable(L, -1))
        {
            int image_index = dclay_AbsIndex(L, -1);

            lua_getfield(L, image_index, "texture");
            if (lua_isnil(L, -1))
            {
                luaL_error(L, "clay image table requires a texture field");
            }
            image.texture = dclay_CheckHashOrString(L, -1, "image.texture");
            lua_pop(L, 1);

            lua_getfield(L, image_index, "animation");
            if (!lua_isnil(L, -1))
            {
                image.animation = dclay_CheckHashOrString(L, -1, "image.animation");
            }
            lua_pop(L, 1);
        }
        else
        {
            image.texture = dclay_CheckHashOrString(L, -1, "image");
        }

        dclay_surface_t* surface = g_ActiveSurface;
        uint32_t         image_index = 0;

        for (; image_index < surface->images.Size(); ++image_index)
        {
            if (surface->images[image_index].texture == image.texture && surface->images[image_index].animation == image.animation)
            {
                break;
            }
        }

        if (image_index == surface->images.Size())
        {
            if (surface->images.Full())
            {
                surface->images.OffsetCapacity(8);
            }

            surface->images.Push(image);
        }

        declaration->image.imageData = (void*)(uintptr_t)(image_index + 1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "clip");
    if (!lua_isnil(L, -1))
    {
        luaL_checktype(L, -1, LUA_TTABLE);

        declaration->clip.horizontal = dclay_GetOptionalBoolField(L, -1, "horizontal", false);
        declaration->clip.vertical = dclay_GetOptionalBoolField(L, -1, "vertical", false);
        declaration->clip.childOffset.x = dclay_GetOptionalNumberField(L, -1, "x", 0);
        declaration->clip.childOffset.y = dclay_GetOptionalNumberField(L, -1, "y", 0);
        use_scroll_offset = dclay_GetOptionalBoolField(L, -1, "scroll", false);

        if (use_scroll_offset && !declaration->clip.horizontal && !declaration->clip.vertical)
        {
            luaL_error(L, "clay clip field 'scroll' requires horizontal and/or vertical clipping");
        }
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "border");
    if (!lua_isnil(L, -1))
    {
        luaL_checktype(L, -1, LUA_TTABLE);

        int border_index = dclay_AbsIndex(L, -1);

        lua_getfield(L, border_index, "color");
        if (!lua_isnil(L, -1))
        {
            dclay_ParseColor(L, -1, &declaration->border.color);
        }
        else
        {
            declaration->border.color = { 255, 255, 255, 255 };
        }
        lua_pop(L, 1);

        lua_getfield(L, border_index, "width");
        if (lua_isnil(L, -1))
        {
            luaL_error(L, "clay border requires a width field");
        }
        dclay_ParseBorderWidth(L, -1, &declaration->border.width);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "transition");
    if (!lua_isnil(L, -1))
    {
        dclay_ParseTransition(L, -1, &declaration->transition);
    }
    lua_pop(L, 1);

    return use_scroll_offset;
}

#if defined(DM_DEBUG)
static void dclay_ValidateNodeIds(lua_State* L, int index)
{
    index = dclay_AbsIndex(L, index);
    if (dclay_HasMetatable(L, index, TEXT_META))
    {
        lua_getfield(L, index, "config");

        lua_getfield(L, -1, "layer");
        if (!lua_isnil(L, -1))
        {
            dclay_CheckHashOrString(L, -1, "layer");
        }
        lua_pop(L, 1);

        lua_getfield(L, -1, "slice9");
        if (!lua_isnil(L, -1))
        {
            dclay_CheckVector4(L, -1, "slice9");
        }
        lua_pop(L, 2);

        return;
    }

    luaL_checktype(L, index, LUA_TTABLE);
    lua_getfield(L, index, "id");
    dclay_ValidateElementId(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "layer");
    if (!lua_isnil(L, -1))
    {
        dclay_CheckHashOrString(L, -1, "layer");
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "slice9");
    if (!lua_isnil(L, -1))
    {
        dclay_CheckVector4(L, -1, "slice9");
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "on_hover");
    if (!lua_isnil(L, -1) && !lua_isfunction(L, -1))
    {
        luaL_error(L, "clay element field 'on_hover' must be a function");
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "children");
    if (!lua_isnil(L, -1))
    {
        luaL_checktype(L, -1, LUA_TTABLE);
        int      children = dclay_AbsIndex(L, -1);

        uint32_t count = (uint32_t)lua_objlen(L, children);
        for (uint32_t i = 1; i <= count; ++i)
        {
            lua_rawgeti(L, children, i);
            dclay_ValidateNodeIds(L, -1);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
}
#endif // DM_DEBUG

static bool dclay_QueryFont(dclay_surface_t* surface, dmhash_t alias, dclay_font_t* out_font)
{
    dmGameSystem::FontResource* resource = (dmGameSystem::FontResource*)dmGui::GetFont(surface->gui_scene, alias);
    if (!resource)
    {
        return false;
    }

    dmGameSystem::FontInfo info = {};
    if (dmGameSystem::ResFontGetInfo(resource, &info) != dmResource::RESULT_OK || info.m_Size == 0 || info.m_Size > 65535)
    {
        return false;
    }

    out_font->alias = alias;
    out_font->base_size = (float)info.m_Size;
    out_font->valid = true;

    return true;
}

static void dclay_ReserveDefaultFont(dclay_surface_t* surface)
{
    // Clay debug mode and internal error texts use font id 0
    // Fall back to using the default font for this.
    dclay_font_t font = {};

    if (!dclay_QueryFont(surface, dmHashString64("Default"), &font))
    {
        dclay_QueryFont(surface, dmHashString64("default"), &font);
    }

    surface->fonts.SetCapacity(4);
    surface->fonts.Push(font);
}

static uint16_t dclay_GetOrCreateFont(lua_State* L, dclay_surface_t* surface, dmhash_t alias)
{
    for (uint32_t i = 0; i < surface->fonts.Size(); ++i)
    {
        if (surface->fonts[i].valid && surface->fonts[i].alias == alias)
        {
            return (uint16_t)i;
        }
    }

    if (surface->fonts.Size() >= 65535)
    {
        luaL_error(L, "Clay surface has too many font bindings");
    }

    dclay_font_t font = {};

    if (!dclay_QueryFont(surface, alias, &font))
    {
        luaL_error(L, "GUI font '%s' is not specified in the scene", dmHashReverseSafe64(alias));
    }

    if (surface->fonts.Full())
    {
        surface->fonts.OffsetCapacity(4);
    }

    surface->fonts.Push(font);

    Clay_ResetMeasureTextCache();

    return (uint16_t)(surface->fonts.Size() - 1);
}

static void dclay_EmitText(lua_State* L, int index)
{
    index = dclay_AbsIndex(L, index);
    lua_getfield(L, index, "text");

    Clay_String            text = dclay_CheckString(L, -1);
    Clay_TextElementConfig config = {};

    lua_getfield(L, index, "config");
    if (!lua_isnil(L, -1))
    {
        luaL_checktype(L, -1, LUA_TTABLE);

        config.userData = dclay_StoreUserData(L, dclay_ParseUserData(L, -1));

        lua_getfield(L, -1, "font_id");
        if (lua_isnil(L, -1))
        {
            luaL_error(L, "clay.text() requires a GUI font_id");
        }
        config.fontId = dclay_GetOrCreateFont(L, g_ActiveSurface, dclay_CheckHashOrString(L, -1, "font_id"));
        lua_pop(L, 1);

        lua_getfield(L, -1, "font_size");
        if (lua_isnil(L, -1))
        {
            config.fontSize = (uint16_t)g_ActiveSurface->fonts[config.fontId].base_size;
        }
        else
        {
            config.fontSize = dclay_ToU16(L, -1, "font_size");
            if (config.fontSize == 0)
            {
                luaL_error(L, "clay text field 'font_size' must be greater than zero");
            }
        }
        lua_pop(L, 1);

        config.letterSpacing = (uint16_t)dclay_GetOptionalNumberField(L, -1, "letter_spacing", 0);
        config.lineHeight = (uint16_t)dclay_GetOptionalNumberField(L, -1, "line_height", 0);

        lua_getfield(L, -1, "text_color");
        if (!lua_isnil(L, -1))
        {
            dclay_ParseColor(L, -1, &config.textColor);
        }
        else
        {
            config.textColor = { 255, 255, 255, 255 };
        }
        lua_pop(L, 1);
    }
    else
    {
        luaL_error(L, "clay.text() requires a config table with font_id");
    }
    lua_pop(L, 1);

    Clay__OpenTextElement(text, config);

    lua_pop(L, 1);
}

static void dclay_PushPointerData(lua_State* L, Clay_PointerData pointer)
{
    lua_createtable(L, 0, 3);

    lua_pushnumber(L, pointer.position.x);
    lua_setfield(L, -2, "x");

    lua_pushnumber(L, pointer.position.y);
    lua_setfield(L, -2, "y");

    lua_pushnumber(L, pointer.state);
    lua_setfield(L, -2, "state");
}

static bool dclay_InvokeHoverCallback(lua_State* L, int index)
{
    // The caller leaves the callback function on top of the Lua stack. Keeping
    // the table lookup outside this helper means elements without on_hover never
    // pay for Clay_Hovered(), which scans the current pointer-over ID array.
    if (!Clay_Hovered())
    {
        lua_pop(L, 1);
        return false;
    }

    index = dclay_AbsIndex(L, index);

    // This callback runs while the native element is open, which is the only
    // point where Clay_Hovered() can resolve IDs local to the current parent.
    // Keep it protected: propagating a Lua longjmp would abandon Clay's open
    // element stack. A failed callback leaves the pre-callback declaration in
    // use for this frame.
    lua_pushvalue(L, index);
    dclay_PushPointerData(L, Clay_GetPointerState());

    return dmScript::PCall(L, 2, 0) == 0;
}

static void dclay_EmitNode(lua_State* L, int index, uint32_t* element_count)
{
    ++(*element_count);

    index = dclay_AbsIndex(L, index);
    if (dclay_HasMetatable(L, index, TEXT_META))
    {
        dclay_EmitText(L, index);
        return;
    }

    luaL_checktype(L, index, LUA_TTABLE);

    Clay_ElementDeclaration declaration = {};
    bool                    use_scroll_offset = dclay_ParseElement(L, index, &declaration);
    dclay_user_data_t       user_data = dclay_ParseUserData(L, index);

    lua_getfield(L, index, "id");
    if (lua_isnil(L, -1))
    {
        Clay__OpenElement();
    }
    else
    {
        Clay__OpenElementWithId(dclay_GetElementId(L, -1));
    }
    lua_pop(L, 1);

    // Parse once before opening so ordinary declaration errors cannot abandon
    // Clay's element stack. If a successful hover callback changes the table,
    // parse it again so those changes affect this same layout.
    lua_getfield(L, index, "on_hover");
    bool has_hover_callback = !lua_isnil(L, -1);
    if (!has_hover_callback)
    {
        lua_pop(L, 1);
    }

    if (has_hover_callback && dclay_InvokeHoverCallback(L, index))
    {
        declaration = {};
        use_scroll_offset = dclay_ParseElement(L, index, &declaration);
        user_data = dclay_ParseUserData(L, index);
    }

    // Clay_GetScrollOffset() is keyed by the currently open element. Lua builds
    // an interim tree before this traversal, so resolve managed offsets here,
    // after opening the element and before applying its declaration.
    if (use_scroll_offset)
    {
        declaration.clip.childOffset = Clay_GetScrollOffset();
    }

    declaration.userData = dclay_StoreUserData(L, user_data);

    Clay__ConfigureOpenElement(declaration);

    lua_getfield(L, index, "children");
    if (!lua_isnil(L, -1))
    {
        luaL_checktype(L, -1, LUA_TTABLE);
        int      children = dclay_AbsIndex(L, -1);
        uint32_t count = (uint32_t)lua_objlen(L, children);
        for (uint32_t i = 1; i <= count; ++i)
        {
            lua_rawgeti(L, children, i);
            dclay_EmitNode(L, -1, element_count);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    Clay__CloseElement();
}

static Clay_Dimensions dclay_MeasureText(Clay_StringSlice text, Clay_TextElementConfig* config, void* user_data)
{
#if defined(DM_DEBUG)
    DM_PROFILE("ClayMeasureText");
    DM_PROPERTY_ADD_U32(rmtp_ClayTextMeasurements, 1);
#endif

    Clay_Dimensions  dimensions = {};
    dclay_surface_t* surface = (dclay_surface_t*)user_data;
    uint32_t         length = (uint32_t)text.length;

    if (!surface || config->fontId >= surface->fonts.Size() || !surface->fonts[config->fontId].valid)
    {
        return dimensions;
    }

    if (surface->text_scratch.Capacity() < length + 1)
    {
        surface->text_scratch.SetCapacity(length + 1);
    }

    surface->text_scratch.SetSize(length + 1);
    memcpy(surface->text_scratch.Begin(), text.chars, length);
    surface->text_scratch[length] = 0;

    const dclay_font_t& font = surface->fonts[config->fontId];
    float               requested_size = config->fontSize > 0 ? (float)config->fontSize : font.base_size;
    float               scale = requested_size / font.base_size;
    float               tracking = requested_size > 0.0f ? (float)config->letterSpacing / requested_size : 0.0f;
    dmGui::TextMetrics  metrics;

    if (dmGui::GetTextMetrics(surface->gui_scene, surface->text_scratch.Begin(), font.alias, FLT_MAX, false, 1.0f, tracking, &metrics) == dmGui::RESULT_OK)
    {
        dimensions.width = metrics.m_Width * scale;
        dimensions.height = (config->lineHeight > 0 ? (float)config->lineHeight : metrics.m_Height * scale);
    }

    return dimensions;
}

static Clay_Color dclay_ApplyOverlays(Clay_Color color, const Clay_Color* overlays, uint32_t overlay_count)
{
    for (uint32_t i = 0; i < overlay_count; ++i)
    {
        const Clay_Color& overlay = overlays[i];
        float             amount = overlay.a / 255.0f;

        color.r += (overlay.r - color.r) * amount;
        color.g += (overlay.g - color.g) * amount;
        color.b += (overlay.b - color.b) * amount;
    }

    return color;
}

static void dclay_GetPivotFactors(dmGui::Pivot pivot, float* x, float* y)
{
    static const float factors[9][2] = {
        { 0.5f, 0.5f },
        { 0.5f, 1.0f },
        { 1.0f, 1.0f },
        { 1.0f, 0.5f },
        { 1.0f, 0.0f },
        { 0.5f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.5f },
        { 0.0f, 1.0f },
    };

    uint32_t index = (uint32_t)pivot;
    if (index >= 9)
    {
        index = 0;
    }

    *x = factors[index][0];
    *y = factors[index][1];
}

static uint64_t dclay_CommandKey(const Clay_RenderCommand& command, dclay_gui_node_type_t type, uint16_t role)
{
    return ((uint64_t)command.id << 32) | ((uint32_t)type << 16) | role;
}

static void dclay_ApplyCommandLayer(dclay_surface_t* surface, dclay_gui_node_t* entry, const Clay_RenderCommand& command)
{
    const dmhash_t     default_layer = dmHashString64("");
    dclay_user_data_t* user_data = (dclay_user_data_t*)command.userData;
    dmhash_t           target_layer = user_data && user_data->layer ? user_data->layer : default_layer;

    if (entry->layer == target_layer)
    {
        return;
    }

    dmGui::Result result = dmGui::SetNodeLayer(surface->gui_scene, entry->node, target_layer);
    if (result == dmGui::RESULT_OK)
    {
        entry->layer = target_layer;
        return;
    }

    if (user_data)
    {
        dmLogError("Clay GUI layer '%s' is not specified in the scene", dmHashReverseSafe64(user_data->layer));
    }

    // Do not leave a retained node on its previous explicit layer when a Lua
    // declaration changes to an invalid one. The default layer always exists
    // and retains Defold's normal parent-layer inheritance behavior.
    if (entry->layer != default_layer && dmGui::SetNodeLayer(surface->gui_scene, entry->node, default_layer) == dmGui::RESULT_OK)
    {
        entry->layer = default_layer;
    }
}

static void dclay_ApplyCommandSlice9(dclay_surface_t* surface, dclay_gui_node_t* entry, const Clay_RenderCommand& command)
{
    dclay_user_data_t* user_data = (dclay_user_data_t*)command.userData;
    dmVMath::Vector4   value = user_data ? user_data->slice9 : dmVMath::Vector4(0.0f);
    if (value.getX() == entry->slice9.getX() &&
        value.getY() == entry->slice9.getY() &&
        value.getZ() == entry->slice9.getZ() &&
        value.getW() == entry->slice9.getW())
    {
        return;
    }

    dmGui::SetNodeProperty(surface->gui_scene, entry->node, dmGui::PROPERTY_SLICE9, value);
    entry->slice9 = value;
}

static bool dclay_GetImageShapeUvTransform(dclay_surface_t* surface, dclay_gui_node_t* entry, dmVMath::Vector4* transform_x, dmVMath::Vector4* transform_y)
{
    *transform_x = dmVMath::Vector4(1.0f, 0.0f, 0.0f, 0.0f);
    *transform_y = dmVMath::Vector4(0.0f, 1.0f, 0.0f, 0.0f);
    const float* uv = dmGui::GetNodeFlipbookAnimUV(surface->gui_scene, entry->node);
    if (!uv)
    {
        return true;
    }

    // GUI atlas UVs are ordered bottom-left, top-left, top-right,
    // bottom-right. Invert the affine mapping from node-local coordinates to
    // atlas coordinates so the fragment shader receives a stable 0..1 shape
    // coordinate independently of atlas placement or 90-degree packing.
    float origin_x = uv[0];
    float origin_y = uv[1];
    float axis_x_x = uv[6] - origin_x;
    float axis_x_y = uv[7] - origin_y;
    float axis_y_x = uv[2] - origin_x;
    float axis_y_y = uv[3] - origin_y;
    float determinant = axis_x_x * axis_y_y - axis_y_x * axis_x_y;
    if (dmMath::Abs(determinant) < 0.000001f)
    {
        return false;
    }

    float inverse_determinant = 1.0f / determinant;
    float x_u = axis_y_y * inverse_determinant;
    float x_v = -axis_y_x * inverse_determinant;
    float y_u = -axis_x_y * inverse_determinant;
    float y_v = axis_x_x * inverse_determinant;
    float x_offset = -(x_u * origin_x + x_v * origin_y);
    float y_offset = -(y_u * origin_x + y_v * origin_y);

    bool  flip_x = false;
    bool  flip_y = false;

    dmGui::GetNodeFlipbookAnimUVFlip(surface->gui_scene, entry->node, flip_x, flip_y);
    if (flip_x)
    {
        x_u = -x_u;
        x_v = -x_v;
        x_offset = 1.0f - x_offset;
    }

    if (flip_y)
    {
        y_u = -y_u;
        y_v = -y_v;
        y_offset = 1.0f - y_offset;
    }

    *transform_x = dmVMath::Vector4(x_u, x_v, x_offset, 0.0f);
    *transform_y = dmVMath::Vector4(y_u, y_v, y_offset, 0.0f);

    return true;
}

static void dclay_SetNodeRoundedRectConstants(dclay_surface_t* surface, dclay_gui_node_t* entry, const Clay_CornerRadius& radius, const dmVMath::Vector4* shape_uv_transform_x = 0, const dmVMath::Vector4* shape_uv_transform_y = 0)
{
    static const dmhash_t corner_radii_hash = dmHashString64("corner_radii");
    static const dmhash_t transform_x_hash = dmHashString64("shape_uv_transform_x");
    static const dmhash_t transform_y_hash = dmHashString64("shape_uv_transform_y");

    // Defold GUI box UVs start at the lower-left. Swizzle Clay's top-left-origin
    // order once, at the point where the shader constant is constructed:
    // bottom-left, bottom-right, top-right, top-left.
    dmVMath::Vector4 corner_radii(radius.bottomLeft, radius.bottomRight, radius.topRight, radius.topLeft);
    bool             rounded = radius.topLeft != 0.0f || radius.topRight != 0.0f || radius.bottomRight != 0.0f || radius.bottomLeft != 0.0f;
    bool             transform_shape_uv = rounded && shape_uv_transform_x && shape_uv_transform_y;

    if (entry->rounded_rect_constants_set == rounded && entry->shape_uv_transform_set == transform_shape_uv && dclay_Vector4Equal(entry->corner_radii, corner_radii) && (!transform_shape_uv || (dclay_Vector4Equal(entry->shape_uv_transform_x, *shape_uv_transform_x) && dclay_Vector4Equal(entry->shape_uv_transform_y, *shape_uv_transform_y))))
    {
        return;
    }

    dmGameSystem::HComponentRenderConstants constants = (dmGameSystem::HComponentRenderConstants)dmGui::GetNodeRenderConstants(surface->gui_scene, entry->node);

    if (!rounded)
    {
        if (constants)
        {
            dmGameSystem::ClearRenderConstant(constants, corner_radii_hash);
            dmGameSystem::ClearRenderConstant(constants, transform_x_hash);
            dmGameSystem::ClearRenderConstant(constants, transform_y_hash);

            if (dmGameSystem::GetRenderConstantCount(constants) == 0)
            {
                dmGui::SetNodeRenderConstants(surface->gui_scene, entry->node, 0);
                dmGameSystem::DestroyRenderConstants(constants);
            }
        }
    }
    else
    {
        if (!constants)
        {
            constants = dmGameSystem::CreateRenderConstants();
            dmGui::SetNodeRenderConstants(surface->gui_scene, entry->node, constants);
        }

        dmGameSystem::SetRenderConstant(constants, corner_radii_hash, &corner_radii, 1);

        if (transform_shape_uv)
        {
            dmVMath::Vector4 transform_x = *shape_uv_transform_x;
            dmVMath::Vector4 transform_y = *shape_uv_transform_y;
            dmGameSystem::SetRenderConstant(constants, transform_x_hash, &transform_x, 1);
            dmGameSystem::SetRenderConstant(constants, transform_y_hash, &transform_y, 1);
        }
        else if (entry->shape_uv_transform_set)
        {
            dmGameSystem::ClearRenderConstant(constants, transform_x_hash);
            dmGameSystem::ClearRenderConstant(constants, transform_y_hash);
        }
    }

    entry->corner_radii = corner_radii;
    entry->rounded_rect_constants_set = rounded;
    entry->shape_uv_transform_set = transform_shape_uv;
    entry->shape_uv_transform_x = transform_shape_uv ? *shape_uv_transform_x : dmVMath::Vector4(0.0f);
    entry->shape_uv_transform_y = transform_shape_uv ? *shape_uv_transform_y : dmVMath::Vector4(0.0f);
}

static bool dclay_NewGuiNode(dclay_surface_t* surface, dclay_gui_node_type_t type, dmGui::HNode parent, dmGui::HNode* out_node)
{
    dmGui::NodeType node_type = type == DCLAY_GUI_NODE_TEXT ? dmGui::NODE_TYPE_TEXT : dmGui::NODE_TYPE_BOX;
    dmGui::HNode    node = dmGui::NewNode(surface->gui_scene, dmVMath::Point3(0.0f, 0.0f, 0.0f), dmVMath::Vector3(1.0f, 1.0f, 0.0f), node_type, 0);
    if (node == dmGui::INVALID_HANDLE)
    {
        return false;
    }

    if (dmGui::SetNodeParent(surface->gui_scene, node, parent, false) != dmGui::RESULT_OK)
    {
        dmGui::DeleteNode(surface->gui_scene, node);
        return false;
    }

    dmGui::SetNodePivot(surface->gui_scene, node, dmGui::PIVOT_NW);
    dmGui::SetNodeAdjustMode(surface->gui_scene, node, dmGui::ADJUST_MODE_STRETCH);
    dmGui::SetNodeInheritAlpha(surface->gui_scene, node, false);
    dmGui::SetNodeSizeMode(surface->gui_scene, node, dmGui::SIZE_MODE_MANUAL);

    *out_node = node;

    return true;
}

static dclay_gui_node_t* dclay_GetOrCreateGuiNode(dclay_surface_t* surface, const Clay_RenderCommand& command, dclay_gui_node_type_t type, uint16_t role, dmGui::HNode parent)
{
    uint64_t          key = dclay_CommandKey(command, type, role);
    dclay_gui_node_t* entry = surface->gui_nodes.Get(key);

    if (entry)
    {
        entry->generation = surface->generation;

        if (dmGui::GetNodeParent(surface->gui_scene, entry->node) != parent)
        {
            dmGui::SetNodeParent(surface->gui_scene, entry->node, parent, false);
        }

        dclay_ApplyCommandLayer(surface, entry, command);

#if defined(DM_DEBUG)
        DM_PROPERTY_ADD_U32(rmtp_ClayNodesReused, 1);
#endif

        return entry;
    }

    if (surface->gui_nodes.Full())
    {
        dmLogError("Clay retained GUI-node table exhausted (%u entries)", surface->gui_nodes.Capacity());
        return 0;
    }

    dclay_gui_node_t new_entry = {};

    if (!dclay_NewGuiNode(surface, type, parent, &new_entry.node))
    {
        return 0;
    }

    new_entry.generation = surface->generation;
    new_entry.type = type;
    new_entry.layer = dmHashString64("");
    new_entry.slice9 = dmVMath::Vector4(0.0f);
    new_entry.corner_radii = dmVMath::Vector4(0.0f);
    new_entry.shape_uv_transform_x = dmVMath::Vector4(0.0f);
    new_entry.shape_uv_transform_y = dmVMath::Vector4(0.0f);

    surface->gui_nodes.Put(key, new_entry);
    entry = surface->gui_nodes.Get(key);

    dclay_ApplyCommandLayer(surface, entry, command);

#if defined(DM_DEBUG)
    dclay_RecordNodesAdded(1);
#endif

    return entry;
}

static void dclay_SetCommandTransform(dclay_surface_t* surface, dmGui::HNode node, const Clay_BoundingBox& box, float scale, const Clay_BoundingBox* parent_box)
{
    float x;
    float y;
    float box_width = box.width;
    float box_height = box.height;
    float scale_x = scale;
    float scale_y = scale;

    if (parent_box)
    {
        x = box.x - parent_box->x;
        y = -(box.y - parent_box->y);
    }
    else
    {
        Clay_Vector2    root_scale = surface->root_screen_scale;
        dmVMath::Point3 root_size = dmGui::GetNodeSize(surface->gui_scene, surface->root_node);
        float           pivot_x, pivot_y;

        dclay_GetPivotFactors(dmGui::GetNodePivot(surface->gui_scene, surface->root_node), &pivot_x, &pivot_y);

        x = -pivot_x * root_size.getX() + box.x / root_scale.x;
        y = (1.0f - pivot_y) * root_size.getY() - box.y / root_scale.y;

        scale_x /= root_scale.x;
        scale_y /= root_scale.y;
    }

    // Note that this doesn't work on texts with font_size differing from GUI Font size.
    if (g_PixelPerfect)
    {
        x = floorf(x);
        y = floorf(y);
        box_width = floorf(box_width);
        box_height = floorf(box_height);
    }

    dmGui::SetNodeProperty(surface->gui_scene, node, dmGui::PROPERTY_POSITION, dmVMath::Vector4(x, y, 0, 1.0f));
    dmGui::SetNodeProperty(surface->gui_scene, node, dmGui::PROPERTY_SIZE, dmVMath::Vector4(box_width / scale, box_height / scale, 0.0f, 0.0f));
    dmGui::SetNodeProperty(surface->gui_scene, node, dmGui::PROPERTY_SCALE, dmVMath::Vector4(scale_x, scale_y, 1.0f, 0.0f));
}

STRUCT_ALIGN(4)
struct dclay_scope_t
{
    dmGui::HNode     parent;
    dmGui::HNode     previous;
    Clay_BoundingBox box;
    int16_t          z_index;
    bool             has_box;
    PAD(1);
};

static const Clay_BoundingBox* dclay_GetScopeBox(const dclay_scope_t& scope)
{
    return scope.has_box ? &scope.box : 0;
}

static bool dclay_SetNodeImage(dclay_surface_t* surface, dclay_gui_node_t* entry, dmhash_t texture, dmhash_t animation)
{
    if (entry->texture == texture && entry->animation == animation)
    {
        return true;
    }

    if (texture == 0)
    {
        dmGui::SetNodeTexture(surface->gui_scene, entry->node, (dmhash_t)0);
        entry->texture = 0;
        entry->animation = 0;
        return true;
    }

    if (dmGui::SetNodeTexture(surface->gui_scene, entry->node, texture) != dmGui::RESULT_OK)
    {
        dmLogError("Clay GUI texture '%s' is not specified in the scene", dmHashReverseSafe64(texture));
        entry->texture = 0;
        entry->animation = 0;
        return false;
    }

    if (animation != 0)
    {
        if (dmGui::PlayNodeFlipbookAnim(surface->gui_scene, entry->node, animation, 0.0f, 1.0f) != dmGui::RESULT_OK)
        {
            dmLogError("Clay GUI atlas animation '%s' was not found", dmHashReverseSafe64(animation));
            entry->texture = texture;
            entry->animation = 0;
            return false;
        }

        if (dmGui::GetNodeAnimationFrameCount(surface->gui_scene, entry->node) == 1)
        {
            dmGui::CancelNodeFlipbookAnim(surface->gui_scene, entry->node, true);
        }
    }

    entry->texture = texture;
    entry->animation = animation;

    return true;
}

static void dclay_OrderGuiNode(dclay_surface_t* surface, dmGui::HNode parent, dmGui::HNode node, dmGui::HNode previous)
{
    if (previous != dmGui::INVALID_HANDLE)
    {
        dmGui::MoveNodeAbove(surface->gui_scene, node, previous);
        return;
    }

    dmGui::HNode first = dmGui::GetFirstChildNode(surface->gui_scene, parent);

    if (first != dmGui::INVALID_HANDLE && first != node)
    {
        dmGui::MoveNodeBelow(surface->gui_scene, node, first);
    }
}

static bool dclay_UpdateRectangleNode(dclay_surface_t* surface, const Clay_RenderCommand& command, dclay_scope_t& scope, const Clay_Color* overlays, uint32_t overlay_count)
{
    dclay_gui_node_t* entry = dclay_GetOrCreateGuiNode(surface, command, DCLAY_GUI_NODE_RECTANGLE, 0, scope.parent);
    if (!entry)
    {
        return false;
    }

    dclay_SetCommandTransform(surface, entry->node, command.boundingBox, 1.0f, dclay_GetScopeBox(scope));
    dclay_SetNodeImage(surface, entry, 0, 0);
    dclay_ApplyCommandSlice9(surface, entry, command);
    dclay_SetNodeRoundedRectConstants(surface, entry, command.renderData.rectangle.cornerRadius);

    Clay_Color color = dclay_ApplyOverlays(command.renderData.rectangle.backgroundColor, overlays, overlay_count);
    dmGui::SetNodeProperty(surface->gui_scene, entry->node, dmGui::PROPERTY_COLOR, dmVMath::Vector4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f));

    dclay_OrderGuiNode(surface, scope.parent, entry->node, scope.previous);

    scope.previous = entry->node;

    return true;
}

static bool dclay_UpdateTextNode(dclay_surface_t* surface, const Clay_RenderCommand& command, dclay_scope_t& scope, const Clay_Color* overlays, uint32_t overlay_count)
{
    const Clay_TextRenderData& text = command.renderData.text;
    if (text.fontId >= surface->fonts.Size() || !surface->fonts[text.fontId].valid)
    {
        return false;
    }

    dclay_gui_node_t* entry = dclay_GetOrCreateGuiNode(surface, command, DCLAY_GUI_NODE_TEXT, 0, scope.parent);
    if (!entry)
    {
        return false;
    }

    const dclay_font_t& font = surface->fonts[text.fontId];
    float               requested_size = text.fontSize > 0 ? (float)text.fontSize : font.base_size;
    float               scale = requested_size / font.base_size;
    float               tracking = requested_size > 0.0f ? (float)text.letterSpacing / requested_size : 0.0f;
    uint32_t            length = (uint32_t)text.stringContents.length;

    if (surface->text_scratch.Capacity() < length + 1)
    {
        surface->text_scratch.SetCapacity(length + 1);
    }

    surface->text_scratch.SetSize(length + 1);
    memcpy(surface->text_scratch.Begin(), text.stringContents.chars, length);
    surface->text_scratch[length] = 0;

    if (dmGui::SetNodeFont(surface->gui_scene, entry->node, font.alias) != dmGui::RESULT_OK)
    {
        return false;
    }

    dmGui::SetNodeText(surface->gui_scene, entry->node, surface->text_scratch.Begin());
    dmGui::SetNodeLineBreak(surface->gui_scene, entry->node, false);
    dmGui::SetNodeTextLeading(surface->gui_scene, entry->node, 1.0f);
    dmGui::SetNodeTextTracking(surface->gui_scene, entry->node, tracking);

    Clay_Color color = dclay_ApplyOverlays(text.textColor, overlays, overlay_count);
    dmGui::SetNodeProperty(surface->gui_scene, entry->node, dmGui::PROPERTY_COLOR, dmVMath::Vector4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f));

    dclay_SetCommandTransform(surface, entry->node, command.boundingBox, scale, dclay_GetScopeBox(scope));
    dclay_OrderGuiNode(surface, scope.parent, entry->node, scope.previous);

    scope.previous = entry->node;

    return true;
}

static bool dclay_UpdateImageNode(dclay_surface_t* surface, const Clay_RenderCommand& command, dclay_scope_t& scope, const Clay_Color* overlays, uint32_t overlay_count)
{
    uintptr_t encoded_index = (uintptr_t)command.renderData.image.imageData;
    if (encoded_index == 0 || encoded_index > surface->images.Size())
    {
        return false;
    }

    const dclay_image_t& image = surface->images[(uint32_t)encoded_index - 1];
    dclay_gui_node_t*    entry = dclay_GetOrCreateGuiNode(surface, command, DCLAY_GUI_NODE_IMAGE, 0, scope.parent);
    if (!entry || !dclay_SetNodeImage(surface, entry, image.texture, image.animation))
    {
        return false;
    }

    Clay_Color color = command.renderData.image.backgroundColor;
    if (color.r == 0.0f && color.g == 0.0f && color.b == 0.0f && color.a == 0.0f)
    {
        color = { 255, 255, 255, 255 };
    }

    color = dclay_ApplyOverlays(color, overlays, overlay_count);
    dmGui::SetNodeProperty(surface->gui_scene, entry->node, dmGui::PROPERTY_COLOR, dmVMath::Vector4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f));

    dclay_ApplyCommandSlice9(surface, entry, command);

    Clay_CornerRadius image_radius = command.renderData.image.cornerRadius;
    bool              has_image_radius = image_radius.topLeft != 0.0f || image_radius.topRight != 0.0f || image_radius.bottomRight != 0.0f || image_radius.bottomLeft != 0.0f;

    if (!dclay_IsZeroVector4(entry->slice9) && has_image_radius)
    {
        // Slice-9 emits nine independently interpolated texture patches. Without
        // a second node-local vertex coordinate there is no continuous shape UV
        // for the SDF, so keep slice-9 and rounding deliberately independent.
        has_image_radius = false;
    }

    dmVMath::Vector4 transform_x;
    dmVMath::Vector4 transform_y;

    if (has_image_radius && dclay_GetImageShapeUvTransform(surface, entry, &transform_x, &transform_y))
    {
        dclay_SetNodeRoundedRectConstants(surface, entry, image_radius, &transform_x, &transform_y);
    }
    else
    {
        dclay_SetNodeRoundedRectConstants(surface, entry, {});
    }

    dclay_SetCommandTransform(surface, entry->node, command.boundingBox, 1.0f, dclay_GetScopeBox(scope));
    dclay_OrderGuiNode(surface, scope.parent, entry->node, scope.previous);

    scope.previous = entry->node;

    return true;
}

static bool dclay_EmitBorderSide(dclay_surface_t* surface, const Clay_RenderCommand& command, dclay_scope_t& scope, const Clay_BoundingBox& box, uint16_t role, Clay_Color color)
{
    if (box.width <= 0.0f || box.height <= 0.0f)
    {
        return false;
    }

    dclay_gui_node_t* entry = dclay_GetOrCreateGuiNode(surface, command, DCLAY_GUI_NODE_BORDER, role, scope.parent);
    if (!entry)
    {
        return false;
    }

    dclay_SetNodeImage(surface, entry, 0, 0);
    dmGui::SetNodeProperty(surface->gui_scene, entry->node, dmGui::PROPERTY_SLICE9, dmVMath::Vector4(0.0f));
    dmGui::SetNodeProperty(surface->gui_scene, entry->node, dmGui::PROPERTY_COLOR, dmVMath::Vector4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f));
    dclay_SetCommandTransform(surface, entry->node, box, 1.0f, dclay_GetScopeBox(scope));
    dclay_OrderGuiNode(surface, scope.parent, entry->node, scope.previous);

    scope.previous = entry->node;

    return true;
}

static void dclay_UpdateBorderNodes(dclay_surface_t* surface, const Clay_RenderCommand& command, dclay_scope_t& scope, const Clay_Color* overlays, uint32_t overlay_count)
{
    const Clay_BorderRenderData& border = command.renderData.border;
    const Clay_BoundingBox&      box = command.boundingBox;
    Clay_Color                   color = dclay_ApplyOverlays(border.color, overlays, overlay_count);
    float                        left = (float)border.width.left;
    float                        right = (float)border.width.right;
    float                        top = (float)border.width.top;
    float                        bottom = (float)border.width.bottom;

    dclay_EmitBorderSide(surface, command, scope, { box.x, box.y, box.width, top }, 1, color);
    dclay_EmitBorderSide(surface, command, scope, { box.x, box.y + box.height - bottom, box.width, bottom }, 2, color);
    dclay_EmitBorderSide(surface, command, scope, { box.x, box.y + top, left, box.height - top - bottom }, 3, color);
    dclay_EmitBorderSide(surface, command, scope, { box.x + box.width - right, box.y + top, right, box.height - top - bottom }, 4, color);
}

static bool dclay_PushClipScope(dclay_surface_t* surface, const Clay_RenderCommand& command, dclay_scope_t* scopes, uint32_t* scope_count)
{
    if (*scope_count >= 9)
    {
        dmLogError("Clay GUI clipping exceeds Defold's maximum stencil nesting depth of 8");
        return false;
    }

    dclay_scope_t&   parent_scope = scopes[*scope_count - 1];
    Clay_BoundingBox box = command.boundingBox;
    Clay_Dimensions  layout = Clay_GetLayoutDimensions();

    // Clay's ordinary clip-element commands populate the per-axis flags, but
    // synthetic scissor commands around clipped floating roots do not. Those
    // commands still carry the inherited clip element's bounding box and, per
    // the render-command contract, are intended to clip to that entire box.
    // Treating { false, false } as two disabled axes would expand this stencil
    // to the full layout and let floating content (notably the debug element
    // hierarchy) draw over adjacent panes.
    bool clip_horizontal = command.renderData.clip.horizontal;
    bool clip_vertical = command.renderData.clip.vertical;
    if (!clip_horizontal && !clip_vertical)
    {
        clip_horizontal = true;
        clip_vertical = true;
    }

    if (!clip_horizontal)
    {
        box.x = 0.0f;
        box.width = layout.width;
    }

    if (!clip_vertical)
    {
        box.y = 0.0f;
        box.height = layout.height;
    }

    dclay_gui_node_t* entry = dclay_GetOrCreateGuiNode(surface, command, DCLAY_GUI_NODE_CLIP, 0, parent_scope.parent);
    if (!entry)
    {
        return false;
    }

    dclay_SetNodeImage(surface, entry, 0, 0);
    dmGui::SetNodeClippingMode(surface->gui_scene, entry->node, dmGui::CLIPPING_MODE_STENCIL);
    dmGui::SetNodeClippingVisible(surface->gui_scene, entry->node, false);
    dmGui::SetNodeClippingInverted(surface->gui_scene, entry->node, false);
    dclay_SetCommandTransform(surface, entry->node, box, 1.0f, dclay_GetScopeBox(parent_scope));
    dclay_OrderGuiNode(surface, parent_scope.parent, entry->node, parent_scope.previous);

    parent_scope.previous = entry->node;

    dclay_scope_t& scope = scopes[(*scope_count)++];
    scope.parent = entry->node;
    scope.previous = dmGui::INVALID_HANDLE;
    scope.box = box;
    scope.z_index = command.zIndex;
    scope.has_box = true;

    return true;
}

static void dclay_DeleteGuiNodes(dclay_surface_t* surface, bool stale_only)
{
    for (;;)
    {
        uint64_t                                  leaf_id = 0;
        bool                                      found_leaf = false;
        dmHashTable64<dclay_gui_node_t>::Iterator entries = surface->gui_nodes.GetIterator();
        while (entries.Next())
        {
            const dclay_gui_node_t& entry = entries.GetValue();
            if (stale_only && entry.generation == surface->generation)
            {
                continue;
            }

            bool                                      has_target_child = false;
            dmHashTable64<dclay_gui_node_t>::Iterator children = surface->gui_nodes.GetIterator();
            while (children.Next() && !has_target_child)
            {
                const dclay_gui_node_t& child = children.GetValue();
                bool                    child_is_current = stale_only && child.generation == surface->generation;
                dmGui::HNode            parent = dmGui::GetNodeParent(surface->gui_scene, child.node);

                if (parent != entry.node)
                {
                    continue;
                }

                if (child_is_current)
                {
                    dmGui::HNode new_parent = dmGui::GetNodeParent(surface->gui_scene, entry.node);
                    dmGui::SetNodeParent(surface->gui_scene, child.node, new_parent, false);
                }
                else
                {
                    has_target_child = true;
                }
            }

            if (!has_target_child)
            {
                leaf_id = entries.GetKey();
                found_leaf = true;
                break;
            }
        }

        if (!found_leaf)
        {
            break;
        }

        dclay_gui_node_t* entry = surface->gui_nodes.Get(leaf_id);
        dmGui::DeleteNode(surface->gui_scene, entry->node);
        surface->gui_nodes.Erase(leaf_id);

#if defined(DM_DEBUG)
        dclay_RecordNodesRemoved(1);
#endif
    }
}

static void dclay_ReconcileGui(dclay_surface_t* surface, Clay_RenderCommandArray commands)
{
    DM_PROFILE("ClayReconcileGui");

    ++surface->generation;

    uint32_t      scope_count = 1;
    dclay_scope_t scopes[9] = {};
    scopes[0].parent = surface->root_node;
    scopes[0].previous = dmGui::INVALID_HANDLE;
    scopes[0].has_box = false;

    uint32_t   overlay_count = 0;
    Clay_Color overlays[16] = {};

    for (int32_t i = 0; i < commands.length; ++i)
    {
        const Clay_RenderCommand& command = commands.internalArray[i];
        dclay_scope_t&            scope = scopes[scope_count - 1];

        switch (command.commandType)
        {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
                dclay_UpdateRectangleNode(surface, command, scope, overlays, overlay_count);
                break;
            case CLAY_RENDER_COMMAND_TYPE_BORDER:
                dclay_UpdateBorderNodes(surface, command, scope, overlays, overlay_count);
                break;
            case CLAY_RENDER_COMMAND_TYPE_TEXT:
                dclay_UpdateTextNode(surface, command, scope, overlays, overlay_count);
                break;
            case CLAY_RENDER_COMMAND_TYPE_IMAGE:
                dclay_UpdateImageNode(surface, command, scope, overlays, overlay_count);
                break;
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
                dclay_PushClipScope(surface, command, scopes, &scope_count);
                break;
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
                if (scope_count > 1)
                {
                    --scope_count;
                }
                else
                {
                    dmLogError("Clay GUI received an unmatched scissor end command");
                }
                break;
            case CLAY_RENDER_COMMAND_TYPE_OVERLAY_COLOR_START:
                if (overlay_count < 16)
                {
                    overlays[overlay_count++] = command.renderData.overlayColor.color;
                }
                else
                {
                    dmLogError("Clay GUI overlay nesting exceeds 16 levels");
                }
                break;
            case CLAY_RENDER_COMMAND_TYPE_OVERLAY_COLOR_END:
                if (overlay_count > 0)
                {
                    --overlay_count;
                }
                else
                {
                    dmLogError("Clay GUI received an unmatched overlay color end command");
                }
                break;
            case CLAY_RENDER_COMMAND_TYPE_NONE:
            case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
                break;
        }
    }

    if (scope_count != 1)
    {
        dmLogError("Clay GUI render commands ended with %u unclosed clipping scopes", scope_count - 1);
    }

    if (overlay_count != 0)
    {
        dmLogError("Clay GUI render commands ended with %u unclosed overlay scopes", overlay_count);
    }

    dclay_DeleteGuiNodes(surface, true);
}

static void dclay_FreeSurface(lua_State* L, dclay_surface_t* surface, bool delete_nodes)
{
    if (!surface || !surface->valid)
    {
        return;
    }

    dclay_ClearRoots(L, surface);

    if (delete_nodes)
    {
        dclay_DeleteGuiNodes(surface, false);
    }

#if defined(DM_DEBUG)
    dclay_RemoveMemoryProfile(surface);

    if (!delete_nodes)
    {
        dclay_RecordNodesRemoved(surface->gui_nodes.Size());
    }

    if (g_SurfaceCount > 0)
    {
        --g_SurfaceCount;
    }
    DM_PROPERTY_SET_U32(rmtp_ClaySurfaces, g_SurfaceCount);
#endif

    surface->gui_nodes.Clear();

    if (Clay_GetCurrentContext() == surface->clay_context)
    {
        Clay_SetCurrentContext(0);
    }

    if (g_ActiveSurface == surface)
    {
        g_ActiveSurface = 0;
    }

    free(surface->clay_memory);

    surface->clay_memory = 0;
    surface->clay_memory_size = 0;
    surface->clay_context = 0;
    surface->valid = false;
}

static int dclay_Initialize(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);

    dmGui::HScene scene = dmGui::LuaCheckScene(L);
    dmGui::HNode  root = dmGui::LuaCheckNode(L, 1);
    lua_Integer   max_element_count = DCLAY_DEFAULT_MAX_ELEMENT_COUNT;

    if (!lua_isnoneornil(L, 2))
    {
        max_element_count = luaL_checkinteger(L, 2);
    }

    // Clay also derives a default text-cache capacity of maxElementCount * 2,
    // so keep the value inside the range that operation can represent.
    if (max_element_count <= 0 || max_element_count > INT32_MAX / 2)
    {
        return DM_LUA_ERROR("clay.initialize() max_element_count must be between 1 and %d", INT32_MAX / 2);
    }

    dclay_surface_t* surface = new dclay_surface_t();
    surface->root_screen_scale = { 1.0f, 1.0f };
    surface->gui_scene = scene;
    surface->root_node = root;
    dclay_ReserveDefaultFont(surface);

    // Max element count determines the capacity of most Clay arrays, so it must
    // be set before asking for the arena size and constructing the context. Do
    // this with no current context to configure only the next context, then
    // restore both Clay's default and the previously selected surface. This
    // prevents one surface's capacity from leaking into another surface.
    Clay_Context* previous_context = Clay_GetCurrentContext();
    Clay_SetCurrentContext(0);
    Clay_SetMaxElementCount((int32_t)max_element_count);

    uint32_t clay_memory_size = Clay_MinMemorySize();
    surface->clay_memory_size = clay_memory_size;
    surface->clay_memory = malloc(clay_memory_size);
    if (!surface->clay_memory)
    {
        Clay_SetMaxElementCount(DCLAY_DEFAULT_MAX_ELEMENT_COUNT);
        Clay_SetCurrentContext(previous_context);
        delete surface;
        return DM_LUA_ERROR("Clay arena alloc failed for %d elements (%u bytes)", (int)max_element_count, clay_memory_size);
    }

    Clay_Arena        arena = Clay_CreateArenaWithCapacityAndMemory(clay_memory_size, surface->clay_memory);
    Clay_Dimensions   dimensions = dclay_GetRootDimensions(surface, &surface->root_screen_scale);
    Clay_ErrorHandler handler = {};
    handler.errorHandlerFunction = dclay_ErrorHandler;

    surface->clay_context = Clay_Initialize(arena, dimensions, handler);
    if (!surface->clay_context)
    {
        Clay_SetCurrentContext(0);
        Clay_SetMaxElementCount(DCLAY_DEFAULT_MAX_ELEMENT_COUNT);
        Clay_SetCurrentContext(previous_context);
        free(surface->clay_memory);
        delete surface;
        return DM_LUA_ERROR("Clay_Initialize failed");
    }

    surface->user_data.SetCapacity((uint32_t)max_element_count);
    surface->gui_nodes.SetCapacity((uint32_t)max_element_count);

    Clay_SetMeasureTextFunction(dclay_MeasureText, surface);
    Clay_SetCurrentContext(0);
    Clay_SetMaxElementCount(DCLAY_DEFAULT_MAX_ELEMENT_COUNT);
    Clay_SetCurrentContext(previous_context);

    surface->valid = true;

#if defined(DM_DEBUG)
    ++g_SurfaceCount;
    DM_PROPERTY_SET_U32(rmtp_ClaySurfaces, g_SurfaceCount);
    dclay_UpdateMemoryProfile(surface);
#endif // DM_DEBUG

    dclay_surface_t** userdata = (dclay_surface_t**)lua_newuserdata(L, sizeof(dclay_surface_t*));
    *userdata = surface;
    luaL_getmetatable(L, SURFACE_META);
    lua_setmetatable(L, -2);
    return 1;
}

static int dclay_BeginLayout(lua_State* L)
{
    DM_PROFILE("ClayBeginLayout");

    DM_LUA_STACK_CHECK(L, 0);

    dclay_surface_t* surface = dclay_CheckSurface(L, 1);
    dclay_SelectSurface(surface);

    // A Lua error can unwind gameplay code between begin_layout() and
    // end_layout(). Clay_BeginLayout() resets its ephemeral per-frame state, so
    // treat the next begin as the recovery boundary for an abandoned layout.
    dclay_ClearRoots(L, surface);

    surface->user_data.SetSize(0);

    // GetNodeSize() is only the authored size. The calculated world transform
    // carries Defold's window/layout adjustment, so use its axis lengths to
    // express the root bounds in screen pixels. Rendered nodes cancel this
    // scale in dclay_SetCommandTransform().
    Clay_SetLayoutDimensions(dclay_GetRootDimensions(surface, &surface->root_screen_scale));

    Clay_BeginLayout();

    surface->layout_open = true;

    return 0;
}

static int dclay_Element(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    dclay_surface_t* surface = g_ActiveSurface;
    if (!surface || !surface->layout_open)
    {
        return DM_LUA_ERROR("clay.element() must be called between clay.begin_layout() and clay.end_layout()");
    }

    luaL_checktype(L, 1, LUA_TTABLE);

    if (surface->root_refs.Full())
    {
        surface->root_refs.OffsetCapacity(8);
    }

    lua_pushvalue(L, 1);

    surface->root_refs.Push(luaL_ref(L, LUA_REGISTRYINDEX));

    return 0;
}

static int dclay_EndLayout(lua_State* L)
{
    DM_PROFILE("ClayEndLayout");

    DM_LUA_STACK_CHECK(L, 0);

    dclay_surface_t* surface = dclay_CheckSurface(L, 1);
    if (surface != g_ActiveSurface || !surface->layout_open)
    {
        return DM_LUA_ERROR("Clay layout not open for this surface");
    }

    dclay_SelectSurface(surface);

#if defined(DM_DEBUG)
    for (uint32_t i = 0; i < surface->root_refs.Size(); ++i)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, surface->root_refs[i]);
        dclay_ValidateNodeIds(L, -1);
        lua_pop(L, 1);
    }
#endif

    uint32_t element_count = 0;

    for (uint32_t i = 0; i < surface->root_refs.Size(); ++i)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, surface->root_refs[i]);
        dclay_EmitNode(L, -1, &element_count);
        lua_pop(L, 1);
    }

    float                   dt = lua_gettop(L) > 1 ? (float)luaL_checknumber(L, 2) : 0.0f;
    Clay_RenderCommandArray commands = Clay_EndLayout(dt);

    surface->layout_open = false;

#if defined(DM_DEBUG)
    DM_PROPERTY_ADD_U32(rmtp_ClayLayouts, 1);
    DM_PROPERTY_ADD_U32(rmtp_ClayElements, element_count);
    DM_PROPERTY_ADD_U32(rmtp_ClayCommands, (uint32_t)commands.length);
#endif

    dclay_ReconcileGui(surface, commands);

#if defined(DM_DEBUG)
    dclay_UpdateMemoryProfile(surface);
#endif

    dclay_ClearRoots(L, surface);

    return 0;
}

static int dclay_NewId(lua_State* L, dclay_id_type_t type)
{
    size_t      length = 0;
    const char* text = luaL_checklstring(L, 1, &length);
    uint32_t    index = (type == DCLAY_ID_GLOBAL_INDEXED || type == DCLAY_ID_LOCAL_INDEXED) ? (uint32_t)luaL_checkinteger(L, 2) : 0;
    dclay_id_t* desc = (dclay_id_t*)lua_newuserdata(L, sizeof(dclay_id_t) + length + 1);
    char*       storage = (char*)(desc + 1);

    desc->type = type;
    desc->index = index;
    desc->string = {};
    desc->string.length = (int32_t)length;
    desc->string.chars = storage;

    memcpy(storage, text, length);
    storage[length] = 0;

    luaL_getmetatable(L, ID_META);
    lua_setmetatable(L, -2);

    return 1;
}

static int dclay_Id(lua_State* L)
{
    return dclay_NewId(L, DCLAY_ID_GLOBAL);
}

static int dclay_Idi(lua_State* L)
{
    return dclay_NewId(L, DCLAY_ID_GLOBAL_INDEXED);
}

static int dclay_IdLocal(lua_State* L)
{
    return dclay_NewId(L, DCLAY_ID_LOCAL);
}

static int dclay_IdiLocal(lua_State* L)
{
    return dclay_NewId(L, DCLAY_ID_LOCAL_INDEXED);
}

static Clay_SizingAxis* dclay_NewSizing(lua_State* L)
{
    Clay_SizingAxis* sizing = (Clay_SizingAxis*)lua_newuserdata(L, sizeof(Clay_SizingAxis));
    *sizing = {};
    luaL_getmetatable(L, SIZING_META);
    lua_setmetatable(L, -2);

    return sizing;
}

static int dclay_SizingFixed(lua_State* L)
{
    float            size = (float)luaL_checknumber(L, 1);
    Clay_SizingAxis* sizing = dclay_NewSizing(L);
    sizing->type = CLAY__SIZING_TYPE_FIXED;
    sizing->size.minMax = { size, size };

    return 1;
}

static int dclay_SizingMinMax(lua_State* L, Clay__SizingType type)
{
    float            min = lua_gettop(L) >= 1 ? (float)luaL_checknumber(L, 1) : 0.0f;
    float            max = lua_gettop(L) >= 2 ? (float)luaL_checknumber(L, 2) : FLT_MAX;
    Clay_SizingAxis* sizing = dclay_NewSizing(L);
    sizing->type = type;
    sizing->size.minMax = { min, max };

    return 1;
}

static int dclay_SizingGrow(lua_State* L)
{
    return dclay_SizingMinMax(L, CLAY__SIZING_TYPE_GROW);
}

static int dclay_SizingFit(lua_State* L)
{
    return dclay_SizingMinMax(L, CLAY__SIZING_TYPE_FIT);
}

static int dclay_SizingPercent(lua_State* L)
{
    float            percent = (float)luaL_checknumber(L, 1);
    Clay_SizingAxis* sizing = dclay_NewSizing(L);
    sizing->type = CLAY__SIZING_TYPE_PERCENT;
    sizing->size.percent = percent;

    return 1;
}

static int dclay_Text(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);

    luaL_checkstring(L, 1);

    if (!lua_isnoneornil(L, 2))
    {
        luaL_checktype(L, 2, LUA_TTABLE);
    }

    lua_createtable(L, 0, 2);

    lua_pushvalue(L, 1);
    lua_setfield(L, -2, "text");

    if (!lua_isnoneornil(L, 2))
    {
        lua_pushvalue(L, 2);
    }
    else
    {
        lua_newtable(L);
    }

    lua_setfield(L, -2, "config");

    luaL_getmetatable(L, TEXT_META);
    lua_setmetatable(L, -2);

    return 1;
}

static Clay_Vector2 dclay_ScreenToLayoutPosition(dclay_surface_t* surface, float screen_x, float screen_y)
{
    dmVMath::Matrix4 inverse_world = Vectormath::Aos::inverse(dmGui::GetNodeWorldTransform(surface->gui_scene, surface->root_node));
    dmVMath::Vector4 local = inverse_world * dmVMath::Vector4(screen_x, screen_y, 0.0f, 1.0f);

    const float      epsilon = 0.0001f;

    // Project the screen-space ray onto the root plane when the root has a 3D rotation, matching Defold's node picking behavior.
    if (dmMath::Abs(local.getZ()) > epsilon)
    {
        dmVMath::Vector4 ray_direction = inverse_world.getCol2();
        if (dmMath::Abs(ray_direction.getZ()) < epsilon)
        {
            return { -FLT_MAX, -FLT_MAX };
        }

        local -= ray_direction * (local.getZ() / ray_direction.getZ());
    }

    Clay_Vector2    root_scale;
    Clay_Dimensions size = dclay_GetRootDimensions(surface, &root_scale);
    float           pivot_x, pivot_y;

    dclay_GetPivotFactors(dmGui::GetNodePivot(surface->gui_scene, surface->root_node), &pivot_x, &pivot_y);

    return {
        local.getX() * root_scale.x + pivot_x * size.width,
        (1.0f - pivot_y) * size.height - local.getY() * root_scale.y
    };
}

static int dclay_SetPointerState(lua_State* L)
{
    DM_PROFILE("ClaySetPointerState");

    DM_LUA_STACK_CHECK(L, 0);

    dclay_surface_t* surface = dclay_CheckSurface(L, 1);
    if (surface->layout_open)
    {
        return DM_LUA_ERROR("clay.set_pointer_state() must be called before clay.begin_layout()");
    }

    float screen_x = (float)luaL_checknumber(L, 2);
    float screen_y = (float)luaL_checknumber(L, 3);

    luaL_checktype(L, 4, LUA_TBOOLEAN);
    bool pressed = lua_toboolean(L, 4) != 0;

    dclay_SelectSurface(surface);

    Clay_Vector2 position = dclay_ScreenToLayoutPosition(surface, screen_x, screen_y);
    Clay_SetPointerState(position, pressed);

    return 0;
}

static int dclay_UpdateScrollContainers(lua_State* L)
{
    DM_PROFILE("ClayUpdateScrollContainers");

    DM_LUA_STACK_CHECK(L, 0);

    dclay_surface_t* surface = dclay_CheckSurface(L, 1);
    if (surface->layout_open)
    {
        return DM_LUA_ERROR("clay.update_scroll_containers() must be called before clay.begin_layout()");
    }

    luaL_checktype(L, 2, LUA_TBOOLEAN);
    bool         enable_drag_scrolling = lua_toboolean(L, 2) != 0;

    Clay_Vector2 scroll_delta = { (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4) };

    float        dt = (float)luaL_checknumber(L, 5);
    if (dt < 0.0f)
    {
        return DM_LUA_ERROR("clay.update_scroll_containers() dt must not be negative");
    }

    dclay_SelectSurface(surface);

    Clay_UpdateScrollContainers(enable_drag_scrolling, scroll_delta, dt);

    return 0;
}

static int dclay_GetPointerState(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);

    dclay_surface_t* surface = dclay_CheckSurface(L, 1);
    dclay_SelectSurface(surface);

    Clay_PointerData pointer = Clay_GetPointerState();
    dclay_PushPointerData(L, pointer);

    return 1;
}

static int dclay_PointerOver(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);

    dclay_surface_t* surface = dclay_CheckSurface(L, 1);
    dclay_SelectSurface(surface);

    Clay_ElementId id = dclay_GetQueryableElementId(L, 2);
    lua_pushboolean(L, Clay_PointerOver(id));

    return 1;
}

static int dclay_GetPointerOverIds(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);

    dclay_surface_t* surface = dclay_CheckSurface(L, 1);
    dclay_SelectSurface(surface);

    Clay_ElementIdArray ids = Clay_GetPointerOverIds();

    lua_createtable(L, ids.length, 0);

    for (int32_t i = 0; i < ids.length; ++i)
    {
        lua_pushnumber(L, Clay_ElementIdArray_Get(&ids, i)->id);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

static int dclay_GetScrollContainerData(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);

    dclay_surface_t* surface = dclay_CheckSurface(L, 1);
    dclay_SelectSurface(surface);

    Clay_ElementId           id = dclay_GetQueryableElementId(L, 2);

    Clay_ScrollContainerData data = Clay_GetScrollContainerData(id);

    if (!data.found)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_createtable(L, 0, 5);

    dmScript::PushVector3(L, dmVMath::Vector3(data.scrollPosition->x, data.scrollPosition->y, 0.0f));
    lua_setfield(L, -2, "position");

    dmScript::PushVector3(L, dmVMath::Vector3(data.scrollContainerDimensions.width, data.scrollContainerDimensions.height, 0.0f));
    lua_setfield(L, -2, "container_size");

    dmScript::PushVector3(L, dmVMath::Vector3(data.contentDimensions.width, data.contentDimensions.height, 0.0f));
    lua_setfield(L, -2, "content_size");

    lua_pushboolean(L, data.config.horizontal);
    lua_setfield(L, -2, "horizontal");

    lua_pushboolean(L, data.config.vertical);
    lua_setfield(L, -2, "vertical");

    return 1;
}

static int dclay_SetScrollPosition(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);

    dclay_surface_t* surface = dclay_CheckSurface(L, 1);
    Clay_ElementId   id = dclay_GetQueryableElementId(L, 2);
    float            x = (float)luaL_checknumber(L, 3);
    float            y = (float)luaL_checknumber(L, 4);

    dclay_SelectSurface(surface);

    Clay_ScrollContainerData data = Clay_GetScrollContainerData(id);
    if (data.found)
    {
        data.scrollPosition->x = x;
        data.scrollPosition->y = y;
    }

    lua_pushboolean(L, data.found);

    return 1;
}

static int dclay_SetCullingEnabled(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    dclay_surface_t* surface = dclay_CheckSurface(L, 1);
    dclay_SelectSurface(surface);

    Clay_SetCullingEnabled(lua_toboolean(L, 2));

    return 0;
}

static int dclay_SetDebugModeEnabled(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    dclay_surface_t* surface = dclay_CheckSurface(L, 1);
    if (surface->layout_open)
    {
        return DM_LUA_ERROR("clay.set_debug_mode_enabled() must be called before clay.begin_layout()");
    }

    dclay_SelectSurface(surface);

    luaL_checktype(L, 2, LUA_TBOOLEAN);
    bool enabled = lua_toboolean(L, 2) != 0;

    if (enabled && (surface->fonts.Empty() || !surface->fonts[0].valid))
    {
        return DM_LUA_ERROR("clay.set_debug_mode_enabled() requires a GUI font named 'Default' or 'default'");
    }

    Clay_SetDebugModeEnabled(enabled);

    return 0;
}

static int dclay_IsDebugModeEnabled(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);

    dclay_surface_t* surface = dclay_CheckSurface(L, 1);
    dclay_SelectSurface(surface);

    lua_pushboolean(L, Clay_IsDebugModeEnabled());

    return 1;
}

static int dclay_Destroy(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    dclay_surface_t** userdata = (dclay_surface_t**)luaL_checkudata(L, 1, SURFACE_META);

    if (userdata && *userdata)
    {
        dclay_FreeSurface(L, *userdata, true);
        delete *userdata;
        *userdata = 0;
    }

    return 0;
}

static int dclay_SurfaceGc(lua_State* L)
{
    dclay_surface_t** userdata = (dclay_surface_t**)luaL_checkudata(L, 1, SURFACE_META);
    if (userdata && *userdata)
    {
        // The GUI scene may already have been destroyed when Lua collects this
        // userdata, so only release Clay-owned memory here. GUI scripts should
        // call clay.destroy(surface) from final().
        dclay_FreeSurface(L, *userdata, false);
        delete *userdata;
        *userdata = 0;
    }

    return 0;
}

static const luaL_reg Module_methods[] = {
    { "initialize", dclay_Initialize },
    { "destroy", dclay_Destroy },
    { "begin_layout", dclay_BeginLayout },
    { "element", dclay_Element },
    { "end_layout", dclay_EndLayout },
    { "text", dclay_Text },
    { "id", dclay_Id },
    { "idi", dclay_Idi },
    { "id_local", dclay_IdLocal },
    { "idi_local", dclay_IdiLocal },
    { "sizing_fixed", dclay_SizingFixed },
    { "sizing_grow", dclay_SizingGrow },
    { "sizing_fit", dclay_SizingFit },
    { "sizing_percent", dclay_SizingPercent },
    { "set_pointer_state", dclay_SetPointerState },
    { "get_pointer_state", dclay_GetPointerState },
    { "hovered", dclay_PointerOver },
    { "pointer_over", dclay_PointerOver },
    { "get_pointer_over_ids", dclay_GetPointerOverIds },
    { "update_scroll_containers", dclay_UpdateScrollContainers },
    { "get_scroll_container_data", dclay_GetScrollContainerData },
    { "set_scroll_position", dclay_SetScrollPosition },
    { "set_culling_enabled", dclay_SetCullingEnabled },
    { "set_debug_mode_enabled", dclay_SetDebugModeEnabled },
    { "is_debug_mode_enabled", dclay_IsDebugModeEnabled },
    { 0, 0 }
};

static void init_lua(lua_State* L)
{
    int top = lua_gettop(L);

    luaL_newmetatable(L, ID_META);
    lua_pop(L, 1);

    luaL_newmetatable(L, SIZING_META);
    lua_pop(L, 1);

    luaL_newmetatable(L, TEXT_META);
    lua_pop(L, 1);

    luaL_newmetatable(L, SURFACE_META);
    lua_pushcfunction(L, dclay_SurfaceGc);
    lua_setfield(L, -2, "__gc");
    lua_pop(L, 1);

    luaL_register(L, MODULE_NAME, Module_methods);

    lua_pushnumber(L, CLAY_LEFT_TO_RIGHT);
    lua_setfield(L, -2, "LEFT_TO_RIGHT");

    lua_pushnumber(L, CLAY_TOP_TO_BOTTOM);
    lua_setfield(L, -2, "TOP_TO_BOTTOM");

    lua_pushnumber(L, CLAY_ALIGN_X_LEFT);
    lua_setfield(L, -2, "ALIGN_X_LEFT");

    lua_pushnumber(L, CLAY_ALIGN_X_RIGHT);
    lua_setfield(L, -2, "ALIGN_X_RIGHT");

    lua_pushnumber(L, CLAY_ALIGN_X_CENTER);
    lua_setfield(L, -2, "ALIGN_X_CENTER");

    lua_pushnumber(L, CLAY_ALIGN_Y_TOP);
    lua_setfield(L, -2, "ALIGN_Y_TOP");

    lua_pushnumber(L, CLAY_ALIGN_Y_BOTTOM);
    lua_setfield(L, -2, "ALIGN_Y_BOTTOM");

    lua_pushnumber(L, CLAY_ALIGN_Y_CENTER);
    lua_setfield(L, -2, "ALIGN_Y_CENTER");

    lua_pushnumber(L, CLAY_ATTACH_POINT_LEFT_TOP);
    lua_setfield(L, -2, "ATTACH_POINT_LEFT_TOP");

    lua_pushnumber(L, CLAY_ATTACH_POINT_LEFT_CENTER);
    lua_setfield(L, -2, "ATTACH_POINT_LEFT_CENTER");

    lua_pushnumber(L, CLAY_ATTACH_POINT_LEFT_BOTTOM);
    lua_setfield(L, -2, "ATTACH_POINT_LEFT_BOTTOM");

    lua_pushnumber(L, CLAY_ATTACH_POINT_CENTER_TOP);
    lua_setfield(L, -2, "ATTACH_POINT_CENTER_TOP");

    lua_pushnumber(L, CLAY_ATTACH_POINT_CENTER_CENTER);
    lua_setfield(L, -2, "ATTACH_POINT_CENTER_CENTER");

    lua_pushnumber(L, CLAY_ATTACH_POINT_CENTER_BOTTOM);
    lua_setfield(L, -2, "ATTACH_POINT_CENTER_BOTTOM");

    lua_pushnumber(L, CLAY_ATTACH_POINT_RIGHT_TOP);
    lua_setfield(L, -2, "ATTACH_POINT_RIGHT_TOP");

    lua_pushnumber(L, CLAY_ATTACH_POINT_RIGHT_CENTER);
    lua_setfield(L, -2, "ATTACH_POINT_RIGHT_CENTER");

    lua_pushnumber(L, CLAY_ATTACH_POINT_RIGHT_BOTTOM);
    lua_setfield(L, -2, "ATTACH_POINT_RIGHT_BOTTOM");

    lua_pushnumber(L, CLAY_POINTER_CAPTURE_MODE_CAPTURE);
    lua_setfield(L, -2, "POINTER_CAPTURE_MODE_CAPTURE");

    lua_pushnumber(L, CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH);
    lua_setfield(L, -2, "POINTER_CAPTURE_MODE_PASSTHROUGH");

    lua_pushnumber(L, CLAY_ATTACH_TO_NONE);
    lua_setfield(L, -2, "ATTACH_TO_NONE");

    lua_pushnumber(L, CLAY_ATTACH_TO_PARENT);
    lua_setfield(L, -2, "ATTACH_TO_PARENT");

    lua_pushnumber(L, CLAY_ATTACH_TO_ELEMENT_WITH_ID);
    lua_setfield(L, -2, "ATTACH_TO_ELEMENT_WITH_ID");

    lua_pushnumber(L, CLAY_ATTACH_TO_ROOT);
    lua_setfield(L, -2, "ATTACH_TO_ROOT");

    lua_pushnumber(L, CLAY_CLIP_TO_NONE);
    lua_setfield(L, -2, "CLIP_TO_NONE");

    lua_pushnumber(L, CLAY_CLIP_TO_ATTACHED_PARENT);
    lua_setfield(L, -2, "CLIP_TO_ATTACHED_PARENT");

    lua_pushnumber(L, CLAY_TRANSITION_PROPERTY_NONE);
    lua_setfield(L, -2, "TRANSITION_PROPERTY_NONE");

    lua_pushnumber(L, CLAY_TRANSITION_PROPERTY_X);
    lua_setfield(L, -2, "TRANSITION_PROPERTY_X");

    lua_pushnumber(L, CLAY_TRANSITION_PROPERTY_Y);
    lua_setfield(L, -2, "TRANSITION_PROPERTY_Y");

    lua_pushnumber(L, CLAY_TRANSITION_PROPERTY_POSITION);
    lua_setfield(L, -2, "TRANSITION_PROPERTY_POSITION");

    lua_pushnumber(L, CLAY_TRANSITION_PROPERTY_WIDTH);
    lua_setfield(L, -2, "TRANSITION_PROPERTY_WIDTH");

    lua_pushnumber(L, CLAY_TRANSITION_PROPERTY_HEIGHT);
    lua_setfield(L, -2, "TRANSITION_PROPERTY_HEIGHT");

    lua_pushnumber(L, CLAY_TRANSITION_PROPERTY_DIMENSIONS);
    lua_setfield(L, -2, "TRANSITION_PROPERTY_DIMENSIONS");

    lua_pushnumber(L, CLAY_TRANSITION_PROPERTY_BOUNDING_BOX);
    lua_setfield(L, -2, "TRANSITION_PROPERTY_BOUNDING_BOX");

    lua_pushnumber(L, CLAY_TRANSITION_PROPERTY_BACKGROUND_COLOR);
    lua_setfield(L, -2, "TRANSITION_PROPERTY_BACKGROUND_COLOR");

    lua_pushnumber(L, CLAY_TRANSITION_PROPERTY_OVERLAY_COLOR);
    lua_setfield(L, -2, "TRANSITION_PROPERTY_OVERLAY_COLOR");

    lua_pushnumber(L, CLAY_TRANSITION_PROPERTY_BORDER_COLOR);
    lua_setfield(L, -2, "TRANSITION_PROPERTY_BORDER_COLOR");

    lua_pushnumber(L, CLAY_TRANSITION_PROPERTY_BORDER_WIDTH);
    lua_setfield(L, -2, "TRANSITION_PROPERTY_BORDER_WIDTH");

    lua_pushnumber(L, CLAY_TRANSITION_PROPERTY_BORDER);
    lua_setfield(L, -2, "TRANSITION_PROPERTY_BORDER");

    lua_pushnumber(L, CLAY_TRANSITION_DISABLE_INTERACTIONS_WHILE_TRANSITIONING_POSITION);
    lua_setfield(L, -2, "TRANSITION_DISABLE_INTERACTIONS_WHILE_TRANSITIONING_POSITION");

    lua_pushnumber(L, CLAY_TRANSITION_ALLOW_INTERACTIONS_WHILE_TRANSITIONING_POSITION);
    lua_setfield(L, -2, "TRANSITION_ALLOW_INTERACTIONS_WHILE_TRANSITIONING_POSITION");

    lua_pushnumber(L, CLAY_POINTER_DATA_PRESSED_THIS_FRAME);
    lua_setfield(L, -2, "POINTER_PRESSED_THIS_FRAME");

    lua_pushnumber(L, CLAY_POINTER_DATA_PRESSED);
    lua_setfield(L, -2, "POINTER_PRESSED");

    lua_pushnumber(L, CLAY_POINTER_DATA_RELEASED_THIS_FRAME);
    lua_setfield(L, -2, "POINTER_RELEASED_THIS_FRAME");

    lua_pushnumber(L, CLAY_POINTER_DATA_RELEASED);
    lua_setfield(L, -2, "POINTER_RELEASED");

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

static dmExtension::Result InitializeDefoldClay(dmExtension::Params* params)
{
    init_lua(params->m_L);

    g_PixelPerfect = ConfigFileGetInt(params->m_ConfigFile, "clay.pixel_perfect", 0);

    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeDefoldClay(dmExtension::Params* params)
{
    (void)params;

    g_ActiveSurface = 0;
    Clay_SetCurrentContext(0);

    return dmExtension::RESULT_OK;
}

#else

static dmExtension::Result InitializeDefoldClay(dmExtension::Params*)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeDefoldClay(dmExtension::Params*)
{
    return dmExtension::RESULT_OK;
}

#endif // DM_HEADLESS

DM_DECLARE_EXTENSION(ClayExt, "ClayExt", 0, 0, InitializeDefoldClay, 0, 0, FinalizeDefoldClay)
