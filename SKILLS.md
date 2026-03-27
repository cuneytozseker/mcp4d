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
- Camera: 50mm, slightly above, orthographic feel
- Parts exploded along insertion axes (use proportional spacing relative to part bounding boxes)
- Fasteners grouped and offset further than body parts
- Optional: thin lines connecting parts to origin positions

## tech-collage-cluster
Style: Dense vertical cluster of tech/industrial slabs, multi-color, editorial
Use when: Abstract hero, tech editorial, futuristic collage, deconstructed aesthetic
- Build in layers: background plane, hero strip, block cluster, panels, rods, details
- Thin orthogonal slabs, 90° rotations only, gaussian distribution for positions
- 3 Z-depth layers: back (dark), mid (main color), front (bright accents)
- Dominant color 50%, secondary 25%, accent colors 15%, dark 10%
- Heatsink/corrugated panels: null with 15–20 thin parallel slats (spacing ~2.5 cm)
- Wide dark panels extending horizontally beyond the cluster for silhouette interest
- Thin black cylinder rods (vertical + horizontal) trailing off top/bottom
- Typography: extruded text splines ("SYSTEM", "KEY", numbers) on dark panels
- Circuit traces: tiny thin cubes in L-shapes, solder dot grids (disc arrays)
- Cross markers (+) on background for editorial feel
- Small area lights, high intensity for hard shadows and depth
- Camera: 100mm+, flat front-on, frame so cluster bleeds off edges
- Cube chamfer sub 1 for edge highlights
