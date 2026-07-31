-- Auto generated from utils/update_script_api.py.
-- WARNING: Do not edit manually. Update the Doxygen comments in extension_clay.cpp.

---@meta
---@diagnostic disable: lowercase-global
---@diagnostic disable: missing-return
---@diagnostic disable: duplicate-doc-param
---@diagnostic disable: duplicate-set-field
---@diagnostic disable: args-after-dots

---@class defold_api.clay
---Lay children out horizontally.
---@field LEFT_TO_RIGHT integer
---Lay children out vertically.
---@field TOP_TO_BOTTOM integer
---Align children to the left.
---@field ALIGN_X_LEFT integer
---Align children to the right.
---@field ALIGN_X_RIGHT integer
---Center children horizontally.
---@field ALIGN_X_CENTER integer
---Align children to the top.
---@field ALIGN_Y_TOP integer
---Align children to the bottom.
---@field ALIGN_Y_BOTTOM integer
---Center children vertically.
---@field ALIGN_Y_CENTER integer
---Left-top floating attachment point.
---@field ATTACH_POINT_LEFT_TOP integer
---Left-center floating attachment point.
---@field ATTACH_POINT_LEFT_CENTER integer
---Left-bottom floating attachment point.
---@field ATTACH_POINT_LEFT_BOTTOM integer
---Center-top floating attachment point.
---@field ATTACH_POINT_CENTER_TOP integer
---Center floating attachment point.
---@field ATTACH_POINT_CENTER_CENTER integer
---Center-bottom floating attachment point.
---@field ATTACH_POINT_CENTER_BOTTOM integer
---Right-top floating attachment point.
---@field ATTACH_POINT_RIGHT_TOP integer
---Right-center floating attachment point.
---@field ATTACH_POINT_RIGHT_CENTER integer
---Right-bottom floating attachment point.
---@field ATTACH_POINT_RIGHT_BOTTOM integer
---Capture pointer interaction from elements behind a floating element.
---@field POINTER_CAPTURE_MODE_CAPTURE integer
---Pass pointer interaction through a floating element.
---@field POINTER_CAPTURE_MODE_PASSTHROUGH integer
---Disable floating behavior.
---@field ATTACH_TO_NONE integer
---Attach a floating element to its declarative parent.
---@field ATTACH_TO_PARENT integer
---Attach a floating element to floating.parent_id.
---@field ATTACH_TO_ELEMENT_WITH_ID integer
---Attach a floating element to the layout root.
---@field ATTACH_TO_ROOT integer
---Do not inherit the attached element's clipping.
---@field CLIP_TO_NONE integer
---Inherit the attached element's clipping.
---@field CLIP_TO_ATTACHED_PARENT integer
---No transition properties.
---@field TRANSITION_PROPERTY_NONE integer
---Transition horizontal position.
---@field TRANSITION_PROPERTY_X integer
---Transition vertical position.
---@field TRANSITION_PROPERTY_Y integer
---Transition both position axes.
---@field TRANSITION_PROPERTY_POSITION integer
---Transition width.
---@field TRANSITION_PROPERTY_WIDTH integer
---Transition height.
---@field TRANSITION_PROPERTY_HEIGHT integer
---Transition width and height.
---@field TRANSITION_PROPERTY_DIMENSIONS integer
---Transition position and dimensions.
---@field TRANSITION_PROPERTY_BOUNDING_BOX integer
---Transition background color.
---@field TRANSITION_PROPERTY_BACKGROUND_COLOR integer
---Transition overlay color.
---@field TRANSITION_PROPERTY_OVERLAY_COLOR integer
---Transition border color.
---@field TRANSITION_PROPERTY_BORDER_COLOR integer
---Transition border widths.
---@field TRANSITION_PROPERTY_BORDER_WIDTH integer
---Transition border color and widths.
---@field TRANSITION_PROPERTY_BORDER integer
---Disable interactions while position is transitioning.
---@field TRANSITION_DISABLE_INTERACTIONS_WHILE_TRANSITIONING_POSITION integer
---Allow interactions while position is transitioning.
---@field TRANSITION_ALLOW_INTERACTIONS_WHILE_TRANSITIONING_POSITION integer
---Pointer changed to pressed this frame.
---@field POINTER_PRESSED_THIS_FRAME integer
---Pointer remains pressed.
---@field POINTER_PRESSED integer
---Pointer changed to released this frame.
---@field POINTER_RELEASED_THIS_FRAME integer
---Pointer remains released.
---@field POINTER_RELEASED integer
clay = {}

---String, Defold hash, or Clay ID descriptor.
---@alias clay.ElementId string|hash|clay.Id

---RGB or RGBA color in Defold's normalized color range.
---@alias clay.Color vector3|vector4

---One radius for every corner, or top-left, top-right, bottom-right, bottom-left.
---@alias clay.CornerRadius number|table<integer, number>

---A child element or text declaration.
---@alias clay.Child clay.Element|clay.Text

---Child layout direction.
---@alias clay.LayoutDirection integer
---| `clay.LEFT_TO_RIGHT`
---| `clay.TOP_TO_BOTTOM`

---Horizontal child alignment.
---@alias clay.AlignX integer
---| `clay.ALIGN_X_LEFT`
---| `clay.ALIGN_X_RIGHT`
---| `clay.ALIGN_X_CENTER`

---Vertical child alignment.
---@alias clay.AlignY integer
---| `clay.ALIGN_Y_TOP`
---| `clay.ALIGN_Y_BOTTOM`
---| `clay.ALIGN_Y_CENTER`

---Floating attachment point.
---@alias clay.AttachPoint integer
---| `clay.ATTACH_POINT_LEFT_TOP`
---| `clay.ATTACH_POINT_LEFT_CENTER`
---| `clay.ATTACH_POINT_LEFT_BOTTOM`
---| `clay.ATTACH_POINT_CENTER_TOP`
---| `clay.ATTACH_POINT_CENTER_CENTER`
---| `clay.ATTACH_POINT_CENTER_BOTTOM`
---| `clay.ATTACH_POINT_RIGHT_TOP`
---| `clay.ATTACH_POINT_RIGHT_CENTER`
---| `clay.ATTACH_POINT_RIGHT_BOTTOM`

---Floating pointer capture behavior.
---@alias clay.PointerCaptureMode integer
---| `clay.POINTER_CAPTURE_MODE_CAPTURE`
---| `clay.POINTER_CAPTURE_MODE_PASSTHROUGH`

---Floating attachment target.
---@alias clay.AttachTo integer
---| `clay.ATTACH_TO_NONE`
---| `clay.ATTACH_TO_PARENT`
---| `clay.ATTACH_TO_ELEMENT_WITH_ID`
---| `clay.ATTACH_TO_ROOT`

---Floating clipping behavior.
---@alias clay.ClipTo integer
---| `clay.CLIP_TO_NONE`
---| `clay.CLIP_TO_ATTACHED_PARENT`

---Nonzero transition property bitmask; add flags to combine them.
---@alias clay.TransitionProperty integer
---| `clay.TRANSITION_PROPERTY_X`
---| `clay.TRANSITION_PROPERTY_Y`
---| `clay.TRANSITION_PROPERTY_POSITION`
---| `clay.TRANSITION_PROPERTY_WIDTH`
---| `clay.TRANSITION_PROPERTY_HEIGHT`
---| `clay.TRANSITION_PROPERTY_DIMENSIONS`
---| `clay.TRANSITION_PROPERTY_BOUNDING_BOX`
---| `clay.TRANSITION_PROPERTY_BACKGROUND_COLOR`
---| `clay.TRANSITION_PROPERTY_OVERLAY_COLOR`
---| `clay.TRANSITION_PROPERTY_BORDER_COLOR`
---| `clay.TRANSITION_PROPERTY_BORDER_WIDTH`
---| `clay.TRANSITION_PROPERTY_BORDER`

---Interaction behavior during position transitions.
---@alias clay.TransitionInteractionHandling integer
---| `clay.TRANSITION_DISABLE_INTERACTIONS_WHILE_TRANSITIONING_POSITION`
---| `clay.TRANSITION_ALLOW_INTERACTIONS_WHILE_TRANSITIONING_POSITION`

---Current Clay pointer state.
---@alias clay.PointerState integer
---| `clay.POINTER_PRESSED_THIS_FRAME`
---| `clay.POINTER_PRESSED`
---| `clay.POINTER_RELEASED_THIS_FRAME`
---| `clay.POINTER_RELEASED`

---A per-root Clay context and retained Defold GUI subtree.
---@class clay.Surface

---Opaque ID descriptor returned by a Clay ID helper.
---@class clay.Id

---Opaque sizing descriptor returned by a Clay sizing helper.
---@class clay.SizingAxis

---Per-side layout padding.
---@class clay.Padding
---@field left? integer Left padding.
---@field right? integer Right padding.
---@field top? integer Top padding.
---@field bottom? integer Bottom padding.

---Element sizing axes.
---@class clay.Sizing
---@field width? clay.SizingAxis Width sizing.
---@field height? clay.SizingAxis Height sizing.

---Alignment of children within an element.
---@class clay.ChildAlignment
---@field x? clay.AlignX Horizontal alignment.
---@field y? clay.AlignY Vertical alignment.

---Clay layout configuration.
---@class clay.Layout
---@field sizing? clay.Sizing Width and height sizing.
---@field padding? integer|clay.Padding One value for every side or per-side padding.
---@field child_gap? integer Gap between children.
---@field layout_direction? clay.LayoutDirection Child layout direction.
---@field child_alignment? clay.ChildAlignment Child alignment.

---Two-dimensional floating offset.
---@class clay.Offset
---@field x? number Horizontal offset.
---@field y? number Vertical offset.

---Additional floating dimensions.
---@class clay.Expand
---@field width? number Additional width.
---@field height? number Additional height.

---Element and parent floating attachment points.
---@class clay.AttachPoints
---@field element? clay.AttachPoint Point on the floating element.
---@field parent? clay.AttachPoint Point on the attachment target.

---Floating element configuration.
---@class clay.Floating
---@field offset? clay.Offset Offset from the attachment point.
---@field expand? clay.Expand Additional dimensions.
---@field parent_id? clay.ElementId Required with ATTACH_TO_ELEMENT_WITH_ID; must be a global ID declared earlier.
---@field z_index? integer Clay floating draw order.
---@field attach_points? clay.AttachPoints Element and parent attachment points.
---@field pointer_capture_mode? clay.PointerCaptureMode Pointer capture behavior.
---@field attach_to? clay.AttachTo Attachment target; defaults to ATTACH_TO_NONE.
---@field clip_to? clay.ClipTo Floating clipping behavior.

---Clipping and optional managed scrolling.
---@class clay.Clip
---@field horizontal? boolean Clip horizontally.
---@field vertical? boolean Clip vertically.
---@field x? number Explicit horizontal child offset.
---@field y? number Explicit vertical child offset.
---@field scroll? boolean Use Clay's managed scroll offset; requires horizontal and/or vertical clipping.

---Per-side border widths.
---@class clay.BorderWidth
---@field left? integer Left width.
---@field right? integer Right width.
---@field top? integer Top width.
---@field bottom? integer Bottom width.
---@field between_children? integer Divider width between children.

---Border configuration.
---@class clay.Border
---@field width integer|clay.BorderWidth One width for every side or per-side widths.
---@field color? clay.Color Border color; defaults to white.
---@field layer? string|hash Explicit GUI layer for the border command; defaults to Defold's unnamed layer.

---Defold GUI image configuration.
---@class clay.Image
---@field texture string|hash GUI texture or atlas name.
---@field animation? string|hash Atlas animation name.

---Native Clay transition configuration.
---@class clay.Transition
---@field duration number Duration in seconds.
---@field properties clay.TransitionProperty Nonzero property bitmask.
---@field interaction_handling? clay.TransitionInteractionHandling Position-transition interaction behavior.

---Pointer coordinates and transition state.
---@class clay.PointerData
---@field x number Layout-space X coordinate.
---@field y number Layout-space Y coordinate.
---@field state clay.PointerState Current pointer state.

---Declarative Clay element. Application-specific data may also be stored on this table.
---@class clay.Element
---@field id? clay.ElementId Missing IDs use a Clay automatic ID.
---@field layout? clay.Layout Layout configuration.
---@field background_color? clay.Color Background color and image tint.
---@field overlay_color? clay.Color Overlay color.
---@field corner_radius? clay.CornerRadius Rounded background or image corners.
---@field floating? clay.Floating Floating configuration.
---@field image? string|hash|clay.Image GUI texture or atlas image.
---@field clip? clay.Clip Clipping configuration.
---@field border? clay.Border Border configuration.
---@field transition? clay.Transition Native Clay transition; stable explicit IDs are recommended.
---@field layer? string|hash Explicit GUI layer for this element's commands; layers are not inherited.
---@field slice9? vector4 Defold slice-9 values ordered left, top, right, bottom.
---@field on_hover? fun(element: clay.Element, pointer?: clay.PointerData) Callback invoked while this native element is open.
---@field children? table<integer, clay.Child> Child elements and text declarations; nil entries are ignored.
---@field [string] any Application data.

---Text layout and Defold rendering configuration.
---@class clay.TextConfig
---@field font_id string|hash Required GUI font name.
---@field font_size? integer Defaults to the size authored in the Defold font resource.
---@field text_color? clay.Color Text color; defaults to white.
---@field letter_spacing? integer Additional letter spacing.
---@field line_height? integer Explicit line height.
---@field layer? string|hash Explicit GUI layer; layers are not inherited.

---Mutable text declaration returned by clay.text().
---@class clay.Text
---@field text string Text content.
---@field config clay.TextConfig Text configuration.

---Retained scroll-container state.
---@class clay.ScrollContainerData
---@field position vector3 Current scroll offset.
---@field container_size vector3 Visible dimensions.
---@field content_size vector3 Content dimensions.
---@field horizontal boolean Whether horizontal clipping is enabled.
---@field vertical boolean Whether vertical clipping is enabled.

---Creates a Clay surface rooted at a Defold GUI node.
---@param root_node node GUI node used as the layout bounds and parent for generated nodes.
---@param max_element_count? integer Element capacity for this surface; defaults to 8192.
---@return clay.Surface surface New Clay surface.
function clay.initialize(root_node, max_element_count) end

---Deletes generated GUI nodes and releases a surface. Call from the GUI script's final() function.
---@param surface clay.Surface Clay surface.
function clay.destroy(surface) end

---Begins a layout and synchronizes its dimensions from the root GUI node.
---@param surface clay.Surface Clay surface.
function clay.begin_layout(surface) end

---Retains a root element declaration for the current layout.
---@param element clay.Element Declarative element tree.
function clay.element(element) end

---Completes the layout and reconciles Clay render commands with retained GUI nodes.
---@param surface clay.Surface Clay surface.
---@param dt? number Frame delta time in seconds; defaults to zero.
function clay.end_layout(surface, dt) end

---Creates a text declaration for an element's children array.
---@param text string Text content.
---@param config clay.TextConfig Text layout and rendering configuration.
---@return clay.Text text_element Mutable text declaration.
function clay.text(text, config) end

---Creates a global Clay string ID.
---@param value string ID string.
---@return clay.Id id Clay ID descriptor.
function clay.id(value) end

---Creates an indexed global Clay ID.
---@param value string Base ID string.
---@param index integer Numeric ID offset.
---@return clay.Id id Clay ID descriptor.
function clay.idi(value, index) end

---Creates a Clay ID local to the currently open parent element.
---@param value string ID string.
---@return clay.Id id Clay ID descriptor.
function clay.id_local(value) end

---Creates an indexed Clay ID local to the currently open parent element.
---@param value string Base ID string.
---@param index integer Numeric ID offset.
---@return clay.Id id Clay ID descriptor.
function clay.idi_local(value, index) end

---Creates a fixed-size axis.
---@param size number Size in layout pixels.
---@return clay.SizingAxis sizing Sizing descriptor.
function clay.sizing_fixed(size) end

---Creates an axis that grows within optional bounds.
---@param min? number Minimum size; defaults to zero.
---@param max? number Maximum size; defaults to unbounded.
---@return clay.SizingAxis sizing Sizing descriptor.
function clay.sizing_grow(min, max) end

---Creates a fit-content axis within optional bounds.
---@param min? number Minimum size; defaults to zero.
---@param max? number Maximum size; defaults to unbounded.
---@return clay.SizingAxis sizing Sizing descriptor.
function clay.sizing_fit(min, max) end

---Creates an axis sized to a fraction of its parent.
---@param percent number Parent-relative size factor.
---@return clay.SizingAxis sizing Sizing descriptor.
function clay.sizing_percent(percent) end

---Updates pointer hit testing for the previous completed layout. Call before begin_layout().
---@param surface clay.Surface Clay surface.
---@param x number Defold screen X coordinate, normally action.screen_x.
---@param y number Defold screen Y coordinate, normally action.screen_y.
---@param pressed boolean Whether the pointer is currently pressed.
function clay.set_pointer_state(surface, x, y, pressed) end

---Returns Clay's current pointer data in layout coordinates.
---@param surface clay.Surface Clay surface.
---@return clay.PointerData pointer Current pointer data.
function clay.get_pointer_state(surface) end

---Returns whether the pointer overlaps a globally identified element in the previous layout.
---@param surface clay.Surface Clay surface.
---@param id clay.ElementId Global element ID; local IDs cannot be queried outside their parent traversal.
---@return boolean hovered True when the pointer overlaps the element.
function clay.hovered(surface, id) end

---Returns whether the pointer overlaps a globally identified element in the previous layout.
---@param surface clay.Surface Clay surface.
---@param id clay.ElementId Global element ID; local IDs cannot be queried outside their parent traversal.
---@return boolean hovered True when the pointer overlaps the element.
function clay.pointer_over(surface, id) end

---Returns the numeric Clay IDs under the current pointer.
---@param surface clay.Surface Clay surface.
---@return table<integer, integer> ids Numeric element IDs in hit-test traversal order.
function clay.get_pointer_over_ids(surface) end

---Updates Clay scroll containers from the previous layout. Call before begin_layout().
---@param surface clay.Surface Clay surface.
---@param enable_drag_scrolling boolean Enable pointer-drag scrolling.
---@param scroll_x number Horizontal scroll delta.
---@param scroll_y number Vertical scroll delta.
---@param dt number Frame delta time in seconds.
function clay.update_scroll_containers(surface, enable_drag_scrolling, scroll_x, scroll_y, dt) end

---Returns retained scroll state for a globally identified clipped element.
---@param surface clay.Surface Clay surface.
---@param id clay.ElementId Global scroll-container ID.
---@return clay.ScrollContainerData|nil data Scroll state, or nil when the container was not found.
function clay.get_scroll_container_data(surface, id) end

---Sets the retained offset of a globally identified scroll container.
---@param surface clay.Surface Clay surface.
---@param id clay.ElementId Global scroll-container ID.
---@param x number Horizontal scroll offset.
---@param y number Vertical scroll offset.
---@return boolean found True when the scroll container was found.
function clay.set_scroll_position(surface, id, x, y) end

---Enables or disables Clay offscreen element culling.
---@param surface clay.Surface Clay surface.
---@param enabled boolean Whether culling is enabled.
function clay.set_culling_enabled(surface, enabled) end

---Enables or disables Clay's built-in debug overlay for one surface.
---@param surface clay.Surface Clay surface.
---@param enabled boolean Whether the debug overlay is enabled. Enabling requires a GUI font named Default or default.
function clay.set_debug_mode_enabled(surface, enabled) end

---Returns whether Clay's built-in debug overlay is enabled.
---@param surface clay.Surface Clay surface.
---@return boolean enabled Whether the debug overlay is enabled.
function clay.is_debug_mode_enabled(surface) end
