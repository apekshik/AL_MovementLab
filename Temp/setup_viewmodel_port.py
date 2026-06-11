"""Port KINEMATION FPS Animation Ultimate viewmodel onto AALCharacter.

Headless setup (run with the editor CLOSED):
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Temp/setup_viewmodel_port.py" \
      -stdout -FullStdOutLogOutput -unattended

Steps:
  1. Introspection dump of the pack's demo pawn (logged, no changes).
  2. Duplicate BP_ViewmodelCharacter -> /Game/Player/BP_AZViewmodel.
  3. Reparent the duplicate onto AALCharacter (brings in UALCharacterMovementComponent).
  4. CDO edits: deactivate the C++ FirstPersonCamera, set EyeHeightComponent to the
     arms mesh, configure the deferred IMC swap.
  5. Duplicate IMC_FPSAnimation -> /Game/Input/IMC_AZViewmodel minus locomotion
     mappings (our legacy C++ input keeps driving movement).
  6. Point /Game/Blueprints/NewGameMode default pawn at the new blueprint.

Revert: re-point NewGameMode default_pawn_class at BP_ALCharacter_C (one CDO edit).
"""

import unreal

PACK_PAWN = '/FPSAnimationPack/Blueprints/Character/BP_ViewmodelCharacter'
PACK_IMC = '/FPSAnimationPack/Input/IMC_FPSAnimation'
NEW_PAWN = '/Game/Player/BP_AZViewmodel'
NEW_IMC = '/Game/Input/IMC_AZViewmodel'
GAME_MODE = '/Game/Blueprints/NewGameMode'
PARENT_CLASS = '/Script/AL_MovementLab.ALCharacter'
# Locomotion stays on the project's legacy bindings; these pack actions must not
# double-drive movement or clobber MaxWalkSpeed.
LOCOMOTION_ACTIONS = {'IA_Move', 'IA_Look', 'IA_Jump', 'IA_Sprint', 'IA_TacSprint'}

eal = unreal.EditorAssetLibrary
ok = []
warnings = []


def log(msg):
    unreal.log('[viewmodel-port] ' + msg)


def fail(msg):
    raise RuntimeError('[viewmodel-port] FATAL: ' + msg)


def save(asset, label):
    if not eal.save_loaded_asset(asset, only_if_is_dirty=False):
        fail('could not save ' + label)
    log('saved ' + label)


_ar = unreal.AssetRegistryHelpers.get_asset_registry()
_ar.wait_for_completion()
# The commandlet's initial scan does not cover plugin content; force it.
_ar.scan_paths_synchronous(['/FPSAnimationPack'], force_rescan=True)

# ---- 1. Introspection dump --------------------------------------------------
if not eal.does_asset_exist(PACK_PAWN):
    fail(PACK_PAWN + ' not found - is the FPS Animation Ultimate plugin enabled?')

src_bp = eal.load_asset(PACK_PAWN)
src_class = unreal.load_object(None, PACK_PAWN + '.BP_ViewmodelCharacter_C')
log('=== BP_ViewmodelCharacter introspection ===')
src_cdo = unreal.get_default_object(src_class)
for comp in src_cdo.get_components_by_class(unreal.ActorComponent):
    log('  CDO component: %s (%s)' % (comp.get_name(), comp.get_class().get_name()))

# ---- 2. Duplicate the pawn --------------------------------------------------
if eal.does_asset_exist(NEW_PAWN):
    log(NEW_PAWN + ' already exists; deleting for a clean re-run')
    eal.delete_asset(NEW_PAWN)

dup_bp = eal.duplicate_asset(PACK_PAWN, NEW_PAWN)
if not dup_bp:
    fail('duplicate_asset failed for ' + PACK_PAWN)
ok.append('duplicated pawn -> ' + NEW_PAWN)

# ---- 3. Reparent onto AALCharacter ------------------------------------------
new_parent = unreal.load_class(None, PARENT_CLASS)
if not new_parent:
    fail('could not load ' + PARENT_CLASS + ' - was the C++ build run first?')

unreal.BlueprintEditorLibrary.reparent_blueprint(dup_bp, new_parent)
ok.append('reparented onto AALCharacter')

# Reload the generated class after reinstancing; old refs are REINST-stale.
gen_class = unreal.load_object(None, NEW_PAWN + '.BP_AZViewmodel_C')
if not gen_class:
    fail('generated class missing after reparent')
cdo = unreal.get_default_object(gen_class)

# ---- 4. CDO edits ------------------------------------------------------------
cam = cdo.get_editor_property('first_person_camera')
if cam:
    cam.set_editor_property('auto_activate', False)
    ok.append('FirstPersonCamera deactivated (pack camera now drives the view)')
else:
    warnings.append('FirstPersonCamera not found on CDO - view may use wrong camera')

mesh = cdo.get_editor_property('mesh')
if mesh:
    cdo.set_editor_property('eye_height_component', mesh)
    ok.append('EyeHeightComponent -> arms mesh (crouch dip moves the socketed camera)')
else:
    warnings.append('Mesh not found on CDO - crouch view dip will be inert')

pack_imc = eal.load_asset(PACK_IMC)
if not pack_imc:
    fail(PACK_IMC + ' not found')

# ---- 5. Trimmed IMC ----------------------------------------------------------
if eal.does_asset_exist(NEW_IMC):
    eal.delete_asset(NEW_IMC)
trimmed = eal.duplicate_asset(PACK_IMC, NEW_IMC)
if not trimmed:
    fail('duplicate_asset failed for ' + PACK_IMC)

mappings = list(trimmed.get_editor_property('mappings'))
kept, dropped = [], []
for m in mappings:
    action = m.get_editor_property('action')
    name = action.get_name() if action else '<none>'
    if name in LOCOMOTION_ACTIONS:
        dropped.append(name)
    else:
        kept.append(m)
trimmed.set_editor_property('mappings', kept)
log('IMC kept %d mappings, dropped: %s' % (len(kept), sorted(set(dropped))))
if not dropped:
    warnings.append('no locomotion mappings found to drop - check IMC action names')
save(trimmed, NEW_IMC)
ok.append('trimmed IMC at ' + NEW_IMC)

cdo.set_editor_property('viewmodel_contexts_to_remove', [pack_imc])
cdo.set_editor_property('viewmodel_contexts_to_add', [trimmed])
ok.append('configured deferred IMC swap on pawn CDO')

unreal.BlueprintEditorLibrary.compile_blueprint(dup_bp)
save(dup_bp, NEW_PAWN)

# ---- 6. Game mode default pawn -----------------------------------------------
gm_class = unreal.load_object(None, GAME_MODE + '.NewGameMode_C')
if not gm_class:
    fail('could not load game mode class at ' + GAME_MODE)
gm_cdo = unreal.get_default_object(gm_class)
old_pawn = gm_cdo.get_editor_property('default_pawn_class')
log('game mode previous default pawn: %s' % (old_pawn.get_name() if old_pawn else 'None'))
gm_cdo.set_editor_property('default_pawn_class', gen_class)
gm_bp = eal.load_asset(GAME_MODE)
save(gm_bp, GAME_MODE)
ok.append('NewGameMode default pawn -> BP_AZViewmodel_C')

# ---- Summary ------------------------------------------------------------------
log('=== DONE ===')
for line in ok:
    log('OK: ' + line)
for line in warnings:
    log('WARNING: ' + line)
