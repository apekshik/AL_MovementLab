"""Creates the weapon FX assets: bullet-hole texture + decal material, tracer material.
Run via: UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<this file>
"""
import os
import unreal

DEST = "/Game/Weapons/Effects"
TEXTURE_PNG = os.path.join(os.environ["TEMP"], "T_BulletHole.png")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

# ---- Import bullet hole texture ----
task = unreal.AssetImportTask()
task.filename = TEXTURE_PNG
task.destination_path = DEST
task.destination_name = "T_BulletHole"
task.automated = True
task.replace_existing = True
task.save = True
asset_tools.import_asset_tasks([task])

tex = unreal.load_asset(DEST + "/T_BulletHole")
if not tex:
    raise RuntimeError("Texture import failed")

# ---- Bullet hole decal material ----
if eal.does_asset_exist(DEST + "/M_BulletHoleDecal"):
    eal.delete_asset(DEST + "/M_BulletHoleDecal")
decal_mat = asset_tools.create_asset("M_BulletHoleDecal", DEST, unreal.Material, unreal.MaterialFactoryNew())
decal_mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
decal_mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

sample = mel.create_material_expression(decal_mat, unreal.MaterialExpressionTextureSample, -400, 0)
sample.set_editor_property("texture", tex)
mel.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(sample, "A", unreal.MaterialProperty.MP_OPACITY)

# Roughen the hole so it doesn't shine
rough = mel.create_material_expression(decal_mat, unreal.MaterialExpressionConstant, -400, 250)
rough.set_editor_property("r", 0.9)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

mel.recompile_material(decal_mat)
eal.save_asset(DEST + "/M_BulletHoleDecal")

# ---- Tracer material: unlit, hot golden emissive ----
if eal.does_asset_exist(DEST + "/M_Tracer"):
    eal.delete_asset(DEST + "/M_Tracer")
tracer_mat = asset_tools.create_asset("M_Tracer", DEST, unreal.Material, unreal.MaterialFactoryNew())
tracer_mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

color = mel.create_material_expression(tracer_mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
# HDR gold: well above 1.0 so bloom kicks in
color.set_editor_property("constant", unreal.LinearColor(20.0, 11.0, 2.5, 1.0))
mel.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

mel.recompile_material(tracer_mat)
eal.save_asset(DEST + "/M_Tracer")

print("FX_ASSETS_OK: " + ", ".join(sorted(a.split("/")[-1] for a in eal.list_assets(DEST))))
