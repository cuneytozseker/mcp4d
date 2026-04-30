# Recipes

## clean-product-hero
Style: White studio, single product, dramatic lighting
Use when: Client wants a beauty shot, product page hero
- Cyclorama (curved white plane, 500cm)
- Octane HDRI (studio, low 0.3)
- Key area light: 45° right, 30° above, warm, intensity 5
- Rim light: behind, cool, intensity 3
- Camera: 85mm, 45° angle, slight DOF
- Floor: shadow catcher material
- Product centered at origin

## dark-moody-tech
Style: Dark background, colored accent lighting, tech product
Use when: Electronics, PCBs, connectors, "futuristic" brief
- Black void background
- Single Octane area light: tight, cold blue, high intensity
- Secondary accent: warm orange from opposite side, low
- Camera: 100mm, tight crop, shallow DOF
- Subtle reflective floor (glossy black, roughness 0.05)

## exploded-technical
Style: Clean white, parts separated, engineering feel
Use when: Client wants to show internals, assembly order
- White void background
- Even, shadowless lighting (HDRI studio, high intensity)
- Camera: 135mm, isometric 3/4 (pitch ~45° down, heading ~37°), classic exploded-view angle showing top/front/side
- Add a Target tag to the camera with the hero model as the target object — this lets CC reposition the camera without manually recalculating rotation
- Explode in two passes:
  1. **Y offsets first** — separate parts vertically along insertion axis, proportional spacing relative to part bounding boxes. Capture viewport to evaluate.
  2. **Z offsets second** — after reviewing the Y explosion, add Z offsets to separate parts that overlap or sit behind each other (e.g. back panels, internal components). Evaluate again.
- Fasteners grouped and offset further than body parts
- Optional: thin lines connecting parts to origin positions

## tech-collage-cluster
Style: Dense vertical cluster of tech/industrial slabs, multi-color, editorial
Use when: Abstract hero, tech editorial, futuristic collage, deconstructed aesthetic
- Build in layers: sky environment, hero strip, block cluster, panels, rods, details
- Background: C4D Sky (5105) + Octane Environment tag (1029643), texture slot `[1150]` = SineWave (1029520) with RgbSpectrum (1029504) color input `[1250]` set to dark grey (~0.07)
- Thin orthogonal slabs, 90° rotations only, gaussian distribution for positions
- 3 Z-depth layers: back (dark), mid (main color), front (bright accents)
- Dominant color 50%, secondary 25%, accent colors 15%, dark 10%
- Heatsink/corrugated panels: null with 15–20 thin parallel slats (spacing ~2.5 cm)
- Wide dark panels extending horizontally beyond the cluster for silhouette interest
- Thin black cylinder rods (vertical + horizontal) trailing off top/bottom
- Typography: extruded text splines ("SYSTEM", "KEY", numbers) on dark panels. Extrude offset `[1009]` = 2–3 cm (default 100 cm is way too much), move `[1002]` Z = 0
- Circuit traces: tiny thin cubes in L-shapes, solder dot grids (disc arrays)
- Technical textures: generate RGBA PNGs with Pillow (PCB traces, data grids, hex patterns, barcodes), apply to **planes** (not blocks) with Octane materials wiring the same image to albedo + opacity (linear gamma 1.0) + emission (low power). Scatter planes across the cluster at various depths. Use cubic mapping (projection 3) on the planes, not flat.
- Cross markers (+) on background for editorial feel
- Small area lights, high intensity for hard shadows and depth
- Camera: 100mm+, flat front-on, frame so cluster bleeds off edges use target tag and add block cluster as the target for guarenteed framing.
- Cube chamfer sub 1 for edge highlights

---

# Utilities

## Poly Counter

Large scenes crash C4D if you call `get_scene_info`, `IsInstanceOf()`, or `GetPolygonCount()` on generators — they force cache evaluation.

**Rules:**
- Only check `obj.GetType() == 5100` (raw polygon, no eval) — never `IsInstanceOf`
- Only call `GetPolygonCount()` on type 5100 objects
- Skip lights (5102), cameras (5103), generators (5159, 5118, 1018544, etc.)
- Safe hierarchy traversal (see CLAUDE.md) — scan one level per `execute_python` call
- Filter early (e.g. >100k polys) to skip small objects

## Delete Matching

Delete all polygon objects with the same bbox dimensions as the selected object(s) within a parent group. Supports single or multi-selection — collects `GetRad()` from each, scans siblings across containers, and removes all matches (including selected). Tolerance 0.001.

## Tex Size Check

Find which textures are hogging VRAM. Disk size ≠ VRAM — a 10 MB PNG at 4K eats 64 MB uncompressed (W×H×4 bytes RGBA).

**Approach:**
1. Walk all materials, collect unique file paths from shaders (param IDs: 1100 for Octane ImageTexture, 1000 for C4D Bitmap)
2. Recurse into sub-shaders with `shader.GetDown()` / `shader.GetNext()`
3. For each path, load with `BaseBitmap().InitWith(path)`, read `GetBw()`/`GetBh()`
4. VRAM estimate = `w * h * 4` bytes per texture
5. Group by resolution bucket to spot the pattern (e.g. 74× 4K textures = 4.6 GB)

**Key numbers:** 1K = 4 MB, 2K = 16 MB, 4K = 64 MB, 8K = 256 MB per texture in VRAM.
