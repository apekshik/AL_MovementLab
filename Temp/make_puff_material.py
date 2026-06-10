"""Creates M_PuffBall: unlit translucent sphere material for muzzle flash / smoke puffs.
Emissive = Color(param) * Fade(param); Opacity = Fade * (1 - Fresnel) * OpacityScale(param)
so the sphere reads as a soft ball, dense in the middle and feathered at the silhouette.
Run via: UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<this file>
"""
import unreal

DEST = "/Game/Weapons/Effects"
NAME = "M_PuffBall"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

if eal.does_asset_exist(DEST + "/" + NAME):
    eal.delete_asset(DEST + "/" + NAME)

mat = asset_tools.create_asset(NAME, DEST, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, -100)
color.set_editor_property("parameter_name", "Color")
color.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

fade = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 150)
fade.set_editor_property("parameter_name", "Fade")
fade.set_editor_property("default_value", 1.0)

op_scale = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 300)
op_scale.set_editor_property("parameter_name", "OpacityScale")
op_scale.set_editor_property("default_value", 0.6)

# Emissive = Color * Fade
emis_mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, -50)
mel.connect_material_expressions(color, "", emis_mul, "A")
mel.connect_material_expressions(fade, "", emis_mul, "B")
mel.connect_material_property(emis_mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

# Opacity = Fade * (1 - Fresnel) * OpacityScale
fresnel = mel.create_material_expression(mat, unreal.MaterialExpressionFresnel, -600, 450)
fresnel.set_editor_property("exponent", 2.5)

one_minus = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, -450, 450)
mel.connect_material_expressions(fresnel, "", one_minus, "")

op_mul1 = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 250)
mel.connect_material_expressions(fade, "", op_mul1, "A")
mel.connect_material_expressions(one_minus, "", op_mul1, "B")

op_mul2 = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 300)
mel.connect_material_expressions(op_mul1, "", op_mul2, "A")
mel.connect_material_expressions(op_scale, "", op_mul2, "B")
mel.connect_material_property(op_mul2, "", unreal.MaterialProperty.MP_OPACITY)

mel.recompile_material(mat)
eal.save_asset(DEST + "/" + NAME)
print("PUFF_MATERIAL_OK")
