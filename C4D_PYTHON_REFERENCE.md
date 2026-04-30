# C4D Python Quick Ref

## Rules
- Always call `c4d.EventAdd()` after scene changes
- Always get doc fresh: `doc = c4d.documents.GetActiveDocument()`
- Angles are radians. Convert: `c4d.utils.Rad(degrees)`, `c4d.utils.Deg(radians)`
- Units are centimeters
- Y-up, left-handed coordinate system

## Object IDs
```
Ocube=5159  Osphere=5160  Ocylinder=5170  Otorus=5163  Ocone=5162
Odisc=5164  Oplane=5168   Otube=5165      Opyramid=5167 Oplatonic=5161
Onull=5140  Ocamera=5103  Olight=5102     Ofloor=5104
Oboole      Oextrude  Osweep  Olathe  Oloft  Osds  Oinstance  Osymmetry
Obend  Otwist  Otaper  Obulge  Offd
```

## MoGraph IDs
```
Omgcloner=1018544  Omgfracture  Omgvoronoifracture  Omgtext  Omgtracerobj
Omgrandomeffector=1018643  Omgplaineffector=1021337
Omgstepeffector  Omgsplineeffector  Omgshadereffector  Omgdelayeffector
```

## Traversal
```python
doc.GetFirstObject()          # first object
doc.SearchObject("name")      # find by name
doc.GetActiveObject()         # selected object
obj.GetDown()                 # first child
obj.GetNext()                 # next sibling
obj.GetUp()                   # parent
obj.GetChildren()             # all children list
doc.GetFirstMaterial()        # first material
doc.SearchMaterial("name")    # find material by name
```

## Transforms
```python
obj.GetAbsPos() / SetAbsPos(Vector)     # position in parent space
obj.GetAbsRot() / SetAbsRot(Vector)     # rotation HPB radians
obj.GetAbsScale() / SetAbsScale(Vector) # scale
obj.GetMg() / SetMg(matrix)             # global matrix
obj.GetMl() / SetMl(matrix)             # local matrix
obj.GetRad()                             # bounding box half-size
obj.GetMp()                              # bounding box center (object space)
```

## Materials
```python
mat = c4d.BaseMaterial(c4d.Mmaterial)         # create standard material
mat[c4d.MATERIAL_COLOR_COLOR] = Vector(r,g,b) # RGB 0-1
tag = obj.MakeTag(c4d.Ttexture)               # assign to object
tag[c4d.TEXTURETAG_MATERIAL] = mat
tag[c4d.TEXTURETAG_PROJECTION] = 3            # Cubic (0=UVW, 1=Spherical, 2=Cylindrical, 3=Cubic, 4=Frontal, 5=Spatial, 6=Flat)
shader = c4d.BaseShader(c4d.Xbitmap)          # bitmap texture
shader[c4d.BITMAPSHADER_FILENAME] = "path.png"
mat.InsertShader(shader)
mat[c4d.MATERIAL_COLOR_SHADER] = shader
```

## Keyframes
```python
track = c4d.CTrack(obj, c4d.DescID(
    c4d.DescLevel(c4d.ID_BASEOBJECT_REL_POSITION, c4d.DTYPE_VECTOR, 0),
    c4d.DescLevel(c4d.VECTOR_X, c4d.DTYPE_REAL, 0)))
obj.InsertTrackSorted(track)
curve = track.GetCurve()
key = curve.AddKey(c4d.BaseTime(frame, fps))["key"]
key.SetValue(curve, value)
key.SetInterpolation(curve, c4d.CINTERPOLATION_SPLINE)
```

## Modeling (SendModelingCommand)
```python
c4d.utils.SendModelingCommand(c4d.MCOMMAND_MAKEEDITABLE, list=[obj], doc=doc)
c4d.utils.SendModelingCommand(c4d.MCOMMAND_CURRENTSTATETOOBJECT, list=[obj], doc=doc)
c4d.utils.SendModelingCommand(c4d.MCOMMAND_ALIGNNORMALS, list=[obj], doc=doc)
c4d.utils.SendModelingCommand(c4d.MCOMMAND_REVERSENORMALS, list=[obj], doc=doc)
c4d.utils.SendModelingCommand(c4d.MCOMMAND_JOIN, list=[obj1, obj2], doc=doc)
c4d.utils.SendModelingCommand(c4d.MCOMMAND_OPTIMIZE, list=[obj], bc=settings, doc=doc)
# Boolean via generator: Oboole, type 0=union 1=subtract 2=intersect
```

## Polygon Access
```python
obj.GetPointCount() / GetPolygonCount()
obj.GetAllPoints()        # list of Vector
obj.GetAllPolygons()      # list of CPolygon (.a .b .c .d)
obj.GetPolygonS()         # polygon selection (BaseSelect)
obj.GetEdgeS()            # edge selection (indexed: 4*poly+edge)
obj.GetPointS()           # point selection
obj.SetPoint(idx, Vector) # then obj.Message(c4d.MSG_UPDATE)
```

## Useful Commands
```python
c4d.CallCommand(12151)    # Frame All
c4d.CallCommand(12108)    # Reload Textures
c4d.CallCommand(12236)    # Select All
c4d.CallCommand(14039)    # Make Editable
c4d.CallCommand(12187)    # Connect + Delete
c4d.CallCommand(12297)    # Render to Picture Viewer
```

## Undo
```python
doc.StartUndo()
doc.AddUndo(c4d.UNDOTYPE_NEW, new_obj)
doc.AddUndo(c4d.UNDOTYPE_CHANGE, existing_obj)  # before modifying
doc.AddUndo(c4d.UNDOTYPE_DELETE, obj)            # before removing
doc.EndUndo()
```

## File I/O
```python
c4d.documents.LoadFile(path)                    # open file
c4d.documents.MergeDocument(doc, path, flags)   # merge into scene
c4d.documents.SaveDocument(doc, path, saveflags, formatid)
# FORMAT_C4DEXPORT, FORMAT_OBJEXPORT, FORMAT_FBXEXPORT, FORMAT_ALEMBICEXPORT
```

## Viewport
```python
bd = doc.GetActiveBaseDraw()
bd.WS(world_pos)          # world to screen
bd.SW(screen_pos)          # screen to world
bd.SetSceneCamera(cam)     # set camera (None for editor cam)
```

## Light Types
```
LIGHT_TYPE_OMNI  LIGHT_TYPE_SPOT  LIGHT_TYPE_AREA  LIGHT_TYPE_INFINITE
LIGHT_BRIGHTNESS  LIGHT_COLOR  LIGHT_SHADOWTYPE_AREA
```

## Render Settings
```python
rd = doc.GetActiveRenderData()
rd[c4d.RDATA_XRES]  rd[c4d.RDATA_YRES]  rd[c4d.RDATA_FRAMERATE]
rd[c4d.RDATA_FRAMEFROM]  rd[c4d.RDATA_FRAMETO]  # BaseTime values
```

## Tag Types
```
Ttexture  Tphong  Tdisplay  Taligntopath  Tprotection  Tcompositing  Txpresso
```

## Full SDK docs
https://developers.maxon.net/docs/py/2026_1_0/index.html
