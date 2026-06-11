"""Wire ballistics + HUD after the combat-parity build (editor closed).

- BP_AZViewmodel.ViewmodelProjectileClass -> BP_ALProjectile_C
- NewGameMode.HUDClass -> AALHUD (C++ crosshair HUD)
"""
import unreal

_ar = unreal.AssetRegistryHelpers.get_asset_registry()
_ar.wait_for_completion()

eal = unreal.EditorAssetLibrary

pawn_bp = eal.load_asset('/Game/Player/BP_AZViewmodel')
pawn_class = unreal.load_object(None, '/Game/Player/BP_AZViewmodel.BP_AZViewmodel_C')
proj_class = unreal.load_object(None, '/Game/BP_ALProjectile.BP_ALProjectile_C')
if not (pawn_bp and pawn_class and proj_class):
    raise RuntimeError('missing pawn or projectile class')

cdo = unreal.get_default_object(pawn_class)
cdo.set_editor_property('viewmodel_projectile_class', proj_class)
unreal.BlueprintEditorLibrary.compile_blueprint(pawn_bp)
if not eal.save_loaded_asset(pawn_bp, only_if_is_dirty=False):
    raise RuntimeError('failed saving pawn BP')
unreal.log('[wire] pawn ViewmodelProjectileClass -> BP_ALProjectile_C')

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

# Re-map IA_Jump so the pack pawn's own jump event fires per press (plays the
# jump arms anim/sound on first, double, and wall jumps). Its Jump() call is
# harmless: no-op mid-air, redundant on the ground.
imc = eal.load_asset('/Game/Input/IMC_AZViewmodel')
ia_jump = eal.load_asset('/FPSAnimationPack/Input/IA_Jump')
if not (imc and ia_jump):
    raise RuntimeError('missing IMC or IA_Jump')
already = [m for m in imc.get_editor_property('mappings')
           if m.get_editor_property('action') and m.get_editor_property('action').get_name() == 'IA_Jump']
if not already:
    key = unreal.Key()
    key.set_editor_property('key_name', 'SpaceBar')
    imc.map_key(ia_jump, key)
    if not eal.save_loaded_asset(imc, only_if_is_dirty=False):
        raise RuntimeError('failed saving IMC')
    unreal.log('[wire] IA_Jump (SpaceBar) re-added to IMC_AZViewmodel')
else:
    unreal.log('[wire] IA_Jump already mapped, skipping')
unreal.log('[wire] DONE')
