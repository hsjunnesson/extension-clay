# Clay for Defold

A native extension that uses [Clay](https://github.com/nicbarker/clay) to lay out native [Defold GUI](https://defold.com/manuals/gui/) nodes.

Clay provides the immediate-mode, declarative layout model. This extension reconciles Clay's render commands into retained Defold GUI nodes, so the result participates in the normal GUI rendering pipeline and can use Defold fonts, atlases, layers, clipping, and input coordinates.

## Installation

Add a release ZIP of this repository to your project's `dependencies` in `game.project`, then fetch libraries. See Defold's [library dependency documentation](https://defold.com/manuals/libraries/) for details.

In the GUI scene that will host the layout:

- Add a box node to use as the Clay root. Its effective on-screen size defines the layout dimensions.
- Set the GUI material to `/clay/materials/gui.material`. This supplies the rounded-rectangle and border shader.
- Add every font, texture, atlas, and layer referenced by the Clay declarations to the GUI scene.
- Raise the GUI scene's `Max Nodes` value when the generated interface can exceed the default node capacity.
- Add a font named `Default` if you want to use Clay's debug overlay.

Set `pixel_perfect = 1` in the `[clay]` section of `game.project` to floor generated node positions and dimensions to whole pixels.

The included [example project](example/) demonstrates fonts, atlas images, layers, clipping and scrolling, hover callbacks, floating tooltips, transitions, nine-slicing, and the debug overlay.

## Basic use

Create and destroy one Clay surface from a GUI script. A surface owns its Clay context and all generated nodes beneath the supplied root.

```lua
function init(self)
    self.surface = clay.initialize(gui.get_node("clay_root"))
end

function update(self, dt)
    clay.begin_layout(self.surface)

    clay.element {
        id = "panel",
        layout = {
            sizing = {
                width = clay.sizing_grow(),
                height = clay.sizing_grow(),
            },
            padding = 16,
            child_gap = 8,
            layout_direction = clay.TOP_TO_BOTTOM,
        },
        corner_radius = 8,
        background_color = vmath.vector4(0.08, 0.1, 0.14, 1),
        layer = hash("background"),
        children = {
            clay.text("Hello from Clay", {
                font_id = hash("heading"),
                font_size = 24,
                text_color = vmath.vector4(1, 1, 1, 1),
                layer = hash("text"),
            }),
        },
    }

    clay.end_layout(self.surface, dt)
end

function final(self)
    clay.destroy(self.surface)
end
```

Pointer and scroll updates operate on the previous completed layout and must happen before `clay.begin_layout()`:

```lua
clay.set_pointer_state(surface, action.screen_x, action.screen_y, pointer_down)
clay.update_scroll_containers(surface, true, scroll_x, scroll_y, dt)
```

The generated [script API](clay/api/clay.script_api) and [Lua annotations](clay/api/clay.lua) provide editor completion and type information for functions, constants, element declarations, and their nested configuration tables.

## Architecture

### Immediate layout, retained rendering

Lua declarations form an intermediate tree between `clay.begin_layout()` and `clay.end_layout()`. At the end of the layout, the extension traverses that tree parents-first and submits it to Clay. The complete render-command array returned by Clay is then treated as authoritative; Clay may also emit commands for features such as its debug overlay and transitions.

Generated GUI nodes are not recreated every frame. They are stored by stable command identity and updated in place. New commands allocate nodes, missing commands remove them, and unchanged commands reuse their existing nodes. Stable explicit element IDs are therefore recommended for interfaces that change structure or use transitions.

### Defold resources stay native

Text refers to fonts already registered on the GUI scene. The same Defold font resource is used for measurement and rendering, and an omitted `font_size` uses the size authored in the font component. Images similarly refer to GUI textures or to a texture-plus-animation pair for atlas regions.

Clay scissor commands become Defold stencil clipping nodes. Because Defold clipping is hierarchy-based, commands inside a clip scope are parented beneath the generated clipping node. Clay supports one-axis clipping; the backend expands the other axis because Defold stencil clipping always clips both.

GUI layers are explicit and are not inherited through the Clay tree. Assigning graphics and text to separate scene layers can turn an alternating sequence of boxes and text into a small number of batches, but it also changes global compositing order.

Clay's border command becomes one full-size, transparent-interior GUI box rather than four thin side nodes. Per-side widths, color, and corner radii are passed to the GUI material, which draws an antialiased SDF border ring. Borders do not use the element's top-level layer: set `border.layer` explicitly when a dedicated border layer is needed, otherwise the border node uses Defold's unnamed layer. Keeping backgrounds and borders on separate layers prevents their differing per-node shader constants from interleaving otherwise batchable backgrounds.

### Fixed surface lifetime

Each surface owns a Clay arena, transient command metadata, resource registries, and a retained-node map. `clay.initialize(root, max_element_count)` preallocates these structures; the default maximum is 8192 elements. Destroy and recreate the surface to change that capacity.

The root node remains the authoritative layout area. Its authored size and effective world scale are sampled before every layout, so window resizing and Defold GUI adjustment are reflected automatically.

## Current limitations

- Transition enter/exit callbacks are not implemented.
- `corner_radius` and `slice9` can't be combined.

## AI disclosure

Parts of this implementation, mostly Lua parsing and documentation, was implemented with LLMs and agentic practises.

## Attribution and license

This project includes a modified copy of [Clay 0.14](https://github.com/nicbarker/clay), created by [Nic Barker](https://github.com/nicbarker) and distributed under the zlib/libpng license. Clay's original copyright and license notice remain in the vendored [`clay.h`](clay/src/clay.h).

The Defold integration is copyright © 2026 Hans Sjunnesson and is released under the [MIT License](LICENSE.md).

## Development

The Lua API documentation is generated from the Doxygen blocks in `clay/src/extension_clay.cpp`. After changing the public API or its documentation, run `python utils/update_script_api.py` from any working directory. Do not edit the generated files in `clay/api` manually.
