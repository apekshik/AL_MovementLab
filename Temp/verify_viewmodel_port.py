import unreal

_ar = unreal.AssetRegistryHelpers.get_asset_registry()
_ar.wait_for_completion()

cls = unreal.load_object(None, '/Game/Player/BP_AZViewmodel.BP_AZViewmodel_C')
cdo = unreal.get_default_object(cls)

unreal.log('[verify] generated class loads: %s' % cls.get_name())

move = cdo.get_editor_property('character_movement')
unreal.log('[verify] movement component class: %s' % move.get_class().get_name())

cam = cdo.get_editor_property('first_person_camera')
unreal.log('[verify] FirstPersonCamera auto_activate: %s' % cam.get_editor_property('auto_activate'))

eye = cdo.get_editor_property('eye_height_component')
unreal.log('[verify] EyeHeightComponent: %s' % (eye.get_name() if eye else 'None'))

rem = cdo.get_editor_property('viewmodel_contexts_to_remove')
add = cdo.get_editor_property('viewmodel_contexts_to_add')
unreal.log('[verify] IMC remove: %s' % [str(c.get_path_name() if c else None) for c in rem])
unreal.log('[verify] IMC add: %s' % [str(c.get_path_name() if c else None) for c in add])

gm = unreal.load_object(None, '/Game/Blueprints/NewGameMode.NewGameMode_C')
gm_cdo = unreal.get_default_object(gm)
pawn = gm_cdo.get_editor_property('default_pawn_class')
unreal.log('[verify] game mode default pawn: %s' % (pawn.get_name() if pawn else 'None'))

imc = unreal.EditorAssetLibrary.load_asset('/Game/Input/IMC_AZViewmodel')
actions = sorted(set(m.get_editor_property('action').get_name() for m in imc.get_editor_property('mappings') if m.get_editor_property('action')))
unreal.log('[verify] trimmed IMC actions: %s' % actions)
