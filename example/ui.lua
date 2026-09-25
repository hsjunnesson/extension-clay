--- UI state and data.
local UI = {
	show_profiler = false,

	---@type clay.Surface?
	clay_surface = nil,
}

--- Commonly used hashes.
---@type table<string, hash>
UI.h = {
	toggle_profiler = hash("key_f1"),
	toggle_clay_debug = hash("key_f2"),
	scroll_up = hash("mouse_wheel_up"),
	scroll_down = hash("mouse_wheel_down"),
	touch = hash("touch"),
	mouse_button_left = hash("mouse_button_left"),
	mouse_button_right = hash("mouse_button_right"),
	heading = hash("heading"),
	label = hash("label"),
	large = hash("large"),
	background = hash("background"),
	borders = hash("borders"),
	text = hash("text"),
	icon = hash("icon"),
	tooltip_background = hash("tooltip_background"),
	tooltip_text = hash("tooltip_text"),
	tooltip_borders = hash("tooltip_borders"),
	example = hash("example"),
	square = hash("square"),
	whep = hash("whep"),
	smiley = hash("smiley"),
	joyful = hash("joyful"),
}

--- Colors
---@type table<string, vector3|vector4>
UI.col = {
	background = vmath.vector3(0.08, 0.1, 0.14),
	neutral_charcoal_violet = vmath.vector3(0x3e / 255.0, 0x35 / 255.0, 0x46 / 255.0),
	neutral_inkstone = vmath.vector3(0x48 / 255.0, 0x4a / 255.0, 0x77 / 255.0),
	blue_powder_sky = vmath.vector3(0x8f / 255.0, 0xd3 / 255.0, 0xff / 255.0),
	blue_ocean_sky = vmath.vector3(0x4d / 255.0, 0x9b / 255.0, 0xe6 / 255.0),
	blue_dusk = vmath.vector3(0x4d / 255.0, 0x65 / 255.0, 0xb4 / 255.0),
	white = vmath.vector3(1.0, 1.0, 1.0),
	black = vmath.vector3(0.0, 0.0, 0.0),
	very_dark_gray = vmath.vector3(0.12, 0.12, 0.12),
	dark_gray = vmath.vector3(0.18, 0.18, 0.18),
	light_gray = vmath.vector3(0.5, 0.5, 0.5),
	light_blue = vmath.vector3(0.38, 0.42, 0.5),
}

--- Inputs pressed this frame, e.g. {hash("mouse_button_1") = true}
---@type table<hash, boolean>
UI.input_pressed = {}

--- Inputs released this frame, e.g. {hash("mouse_button_1") = true}
---@type table<hash, boolean>
UI.input_released = {}

--- Inputs continuously held down this frame, e.g. {hash("mouse_button_1") = true}
---@type table<hash, boolean>
UI.input_down = {}

UI.pointer_state = {-1, -1}

UI.scroll_state = {0, 0}

---@type table<string, clay.TextConfig>
UI.text = {
	joyful = {
		font_id = UI.h.joyful,
		layer = UI.h.text,
		text_color = UI.col.white,
		letter_spacing = 4,
	},
	heading = {
		font_id = UI.h.heading,
		layer = UI.h.text,
		text_color = UI.col.white,
	},
	heading_black = {
		font_id = UI.h.heading,
		layer = UI.h.text,
		text_color = UI.col.black,
	},
	label = {
		font_id = UI.h.label,
		layer = UI.h.text,
		text_color = UI.col.black,
	},
	selected_view = {
		font_id = UI.h.heading,
		layer = UI.h.text,
		text_color = UI.col.white,
	},
	button_label = {
		font_id = UI.h.label,
		layer = UI.h.text,
		text_color = UI.col.white,
	},
	large = {
		font_id = UI.h.large,
		layer = UI.h.text,
		text_color = UI.col.white,
		text_alignment = clay.TEXT_ALIGN_CENTER,
	},
}

function UI:reset_input_state()
	self.scroll_state[1] = 0
	self.scroll_state[2] = 0
	self.input_pressed = {}
	self.input_released = {}
end

--- Table that holds widget callback functions.
---@class WidgetCallbacks
---@field on_pressed? fun(action_id: hash) The callback when an action_id was pressed while hovering
---@field on_released? fun(action_id: hash) The callback when an action_id was released while hovering.
---@field on_down? fun(action_id: hash) The callback when touch was down while hovering.

--- Button widget
--- Requires either `text` or `image` to be non nil. If both, creates a button with text and image icon.
--- If just text, then it's just a text button.
--- If just image, it's just the image.
---@param id clay.ElementId
---@param text? string Optional text string.
---@param image? string Optional image
---@param callbacks? WidgetCallbacks
---@return clay.Element
function UI.button(id, text, image, callbacks)
	assert(text or image)

	---@type clay.Element
	local element = {
		id = id,
		layout = {
			padding = 4,
			child_alignment = { x = clay.ALIGN_X_CENTER, y = clay.ALIGN_Y_CENTER },
			child_gap = 4,
		},
		layer = UI.h.background,
		background_color = UI.col.blue_ocean_sky,
		corner_radius = 8,
		on_hover = function(element)
			element.background_color = UI.col.blue_dusk

			-- Check whether any inputs were pressed while hovering the button.
			if callbacks and callbacks.on_pressed then
				for action_id, pressed in pairs(UI.input_pressed) do
					if pressed then
						callbacks.on_pressed(action_id)

						-- Visual feedback when left clicking or touching button.
						if action_id == UI.h.mouse_button_left or action_id == UI.h.touch then
							element.background_color = UI.col.neutral_inkstone
						end
					end
				end
			end

			-- Check whether any inputs were released while hovering the button.
			if callbacks and callbacks.on_released then
				for action_id, released in pairs(UI.input_released) do
					if released then
						callbacks.on_released(action_id)
					end
				end
			end

			-- Check whether any inputs are held down while hovering the button.
			if callbacks and callbacks.on_down then
				for action_id, down in pairs(UI.input_down) do
					if down then
						callbacks.on_down(action_id)
					end

					-- Visual feedback when left clicking or touching button.
					if action_id == UI.h.mouse_button_left or action_id == UI.h.touch then
						element.background_color = UI.col.neutral_inkstone
					end
				end
			end
		end,
		children = {
		},
	}

	if image then
		---@type clay.Element
		local img = {
			layer = UI.h.icon,
			image = {
				texture = UI.h.example,
				animation = image,
			},
			layout = {
				sizing = {
					width = clay.sizing_fixed(24),
					height = clay.sizing_fixed(24),
				}
			}
		}

		table.insert(element.children, img)
	end

	if text then
		table.insert(element.children, clay.text(text, UI.text.button_label))
	end

	return element
end

return UI
