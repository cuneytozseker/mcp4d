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

## abstract-block-cluster
Style: Dense cluster of thin orthogonal slabs, monochrome, editorial/abstract
Use when: Abstract hero, editorial background, geometric art direction
- 50–90 thin rectangular slabs (one dim 3–10 cm, others 10–250 cm)
- Rotations strictly 90° only — no random angles
- Gaussian distribution for positions, clustered vertically
- Mix: tall thin (35%), medium (30%), wide short (20%), small accent (15%)
- Pick one area and pack 20–30 tiny detail blocks for density contrast
- Cube fillet subdivision 1 (chamfer), radius ~4% of smallest dim — edges catch highlights
- Colored BG light behind cluster (size 15000+, camera visible, no shadows) instead of emissive sphere
- Key area light: large, power 80–120, upper-left
- Fill area light: larger, power 5–10, opposite side
- Camera: 90–105mm, target expression, frame so blocks bleed off edges
- Material: deep saturated diffuse, specular 0.3, roughness 0.06, no transmission
