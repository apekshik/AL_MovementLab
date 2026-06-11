"""Point NewGameMode's HUD class at the C++ crosshair HUD (editor closed)."""
import unreal

unreal.AssetRegistryHelpers.get_asset_registry().wait_for_completion()

eal = unreal.EditorAssetLibrary
hud_class = unreal.load_class(None, '/Script/AL_MovementLab.ALHUD')
gm_bp = eal.load_asset('/Game/Blueprints/NewGameMode')
gm_class = unreal.load_object(None, '/Game/Blueprints/NewGameMode.NewGameMode_C')
if not (hud_class and gm_bp and gm_class):
    raise RuntimeError('missing HUD or game mode class')

gm_cdo = unreal.get_default_object(gm_class)
gm_cdo.set_editor_property('hud_class', hud_class)
unreal.BlueprintEditorLibrary.compile_blueprint(gm_bp)
if not eal.save_loaded_asset(gm_bp, only_if_is_dirty=False):
    raise RuntimeError('failed saving game mode BP')
unreal.log('[wire] game mode HUDClass -> ALHUD')
