#pragma once

// This isn't neat. But a lot of function we require in dmGui aren't available to Defold extensions,
// so we must declare those functions and structs here. If these functions change, we need to make sure changes are carried over.

#if defined(_WIN32) && defined(GetTextMetrics)
#undef GetTextMetrics
#endif

// From font/text_layout.h

enum TextResult
{
    TEXT_RESULT_OK,
    TEXT_RESULT_ERROR,
};

struct TextLayoutObjectAttribute
{
    uint32_t m_NameOffset;
    uint32_t m_ValueOffset;
    uint16_t m_NameLength;
    uint16_t m_ValueLength;
};

struct TextLayoutObject
{
    uintptr_t m_Resource;
    uint64_t  m_Id;
    float     m_Width;
    float     m_Height;
    uint32_t  m_TextOffset;
    uint32_t  m_TextLength;
    uint16_t  m_AttributeIndex;
    uint16_t  m_AttributeCount;
    dmhash_t  m_Tag;
};

typedef uint8_t (*FTextLayoutResolveObject)(void*                            context,
                                            const char*                      source,
                                            const TextLayoutObjectAttribute* attributes,
                                            float                            proposed_width,
                                            float                            proposed_height,
                                            TextLayoutObject*                object);

typedef void (*FTextLayoutReleaseObject)(void* context, const TextLayoutObject* object);

struct TextLayoutSettings
{
    float                    m_Size;
    float                    m_Width;
    float                    m_Leading;
    float                    m_Tracking;

    FTextLayoutResolveObject m_ResolveObject;
    FTextLayoutReleaseObject m_ReleaseObject;
    void*                    m_ObjectContext;

    dmhash_t                 m_BaseStyle;

    uint32_t                 m_Padding;
    uint8_t                  m_LineBreak : 1;
    uint8_t                  m_Monospace : 1;
    uint8_t                  m_UseBaseStyle : 1;
};

typedef struct FontCollection* HFontCollection;
typedef struct TextLayout*     HTextLayout;

// From markup.h

typedef struct Markup* HMarkup;

enum MarkupResult
{
    MARKUP_RESULT_OK,
    MARKUP_RESULT_INCOMPLETE,
    MARKUP_RESULT_SYNTAX_ERROR,
    MARKUP_RESULT_INVALID_UTF8,
    MARKUP_RESULT_LIMIT_EXCEEDED,
    MARKUP_RESULT_UNSUPPORTED,
};

enum MarkupErrorType
{
    MARKUP_ERROR_NONE,
    MARKUP_ERROR_INCOMPLETE_TAG,
    MARKUP_ERROR_INCOMPLETE_ENTITY,
    MARKUP_ERROR_UNCLOSED_TAG,
    MARKUP_ERROR_INVALID_TAG,
    MARKUP_ERROR_INVALID_ATTRIBUTE,
    MARKUP_ERROR_INVALID_ENTITY,
    MARKUP_ERROR_UNEXPECTED_CLOSING_TAG,
    MARKUP_ERROR_MISMATCHED_CLOSING_TAG,
    MARKUP_ERROR_INVALID_UTF8,
    MARKUP_ERROR_LIMIT_EXCEEDED,
    MARKUP_ERROR_UNSUPPORTED,
    MARKUP_ERROR_UNKNOWN_TAG,
    MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
    MARKUP_ERROR_INVALID_ATTRIBUTE_VALUE,
};

struct MarkupError
{
    uint32_t        m_ByteOffset;
    MarkupErrorType m_Type;
};

MarkupResult MarkupCreate(const char* text, uint32_t text_length, HMarkup* out_markup, MarkupError* out_error);
TextResult   TextLayoutCreateMarkup(HFontCollection collection, HMarkup markup, TextLayoutSettings* settings, HTextLayout* outlayout);
void         MarkupDestroy(HMarkup);
void         TextLayoutRelease(HTextLayout layout);

namespace dmRender
{
    struct TextMetrics
    {
        float    m_Width;      /// Total string width
        float    m_Height;     /// Total string height
        float    m_MaxAscent;  /// Max ascent of font
        float    m_MaxDescent; /// Max descent of font, positive value.
        uint32_t m_LineCount;  /// Number of lines of text
    };

    float           GetFontMapSize(dmRender::HFontMap font_map);
    uint32_t        GetFontMapPadding(dmRender::HFontMap font_map);
    bool            GetFontMapMonospaced(dmRender::HFontMap font_map);
    HFontCollection GetFontCollection(dmRender::HFontMap font_map);
    void            GetTextMetrics(HFontMap font_map, const char* text, TextLayoutSettings* settings, TextMetrics* metrics);
    void            GetTextMetrics(HFontMap font_map, HTextLayout layout, TextMetrics* metrics);

} // namespace dmRender

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